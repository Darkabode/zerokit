#include "zgui.h"

#ifdef ZGUI_USE_COMBO

namespace zgui {

class CComboWnd : public Window
{
public:
    void Init(Combo* pOwner);
	const String& GetWindowClassName() const;
    void OnFinalMessage(HWND hWnd);

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void EnsureVisible(int iIndex);
    void Scroll(int dx, int dy);

#if(_WIN32_WINNT >= 0x0501)
	virtual UINT GetClassStyle() const;
#endif

public:
    PaintManager _paintManager;
    Combo* m_pOwner;
    VerticalLayout* m_pLayout;
    int m_iOldSel;

private:
	static const String WINDOW_CLASS_NAME;
};

const String CComboWnd::WINDOW_CLASS_NAME = "ComboWnd";


void CComboWnd::Init(Combo* pOwner)
{
    m_pOwner = pOwner;
    m_pLayout = 0;
    m_iOldSel = m_pOwner->GetCurSel();

    // Position the popup window in absolute space
    SIZE szDrop = m_pOwner->GetDropBoxSize();
    RECT rcOwner = pOwner->GetPos();
    RECT rc = rcOwner;
    rc.top = rc.bottom;		// 父窗口left、bottom位置作为弹出窗口起点
    rc.bottom = rc.top + szDrop.cy;	// 计算弹出窗口高度
    if( szDrop.cx > 0 ) rc.right = rc.left + szDrop.cx;	// 计算弹出窗口宽度

    SIZE szAvailable = { rc.right - rc.left, rc.bottom - rc.top };
    int cyFixed = 0;
    for (int it = 0; it < pOwner->getCount(); ++it) {
        Control* pControl = static_cast<Control*>(pOwner->getItem(it));
		if (!pControl->IsVisible()) {
			continue;
		}
        SIZE sz = pControl->EstimateSize(szAvailable);
        cyFixed += sz.cy;
    }
    cyFixed += 4; // VerticalLayout 默认的Inset 调整
    rc.bottom = rc.top + MIN(cyFixed, szDrop.cy);

	fn_MapWindowPoints(pOwner->getManager()->GetPaintWindow(), HWND_DESKTOP, (LPPOINT)&rc, 2);

    MONITORINFO oMonitor;
	__stosb((uint8_t*)&oMonitor, 0, sizeof(oMonitor));
    oMonitor.cbSize = sizeof(oMonitor);
    fn_GetMonitorInfoW(fn_MonitorFromWindow(*this, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
    Rect rcWork = oMonitor.rcWork;
    if (rc.bottom > rcWork.bottom) {
        rc.left = rcOwner.left;
        rc.right = rcOwner.right;
		if (szDrop.cx > 0) {
			rc.right = rc.left + szDrop.cx;
		}
        rc.top = rcOwner.top - MIN(cyFixed, szDrop.cy);
        rc.bottom = rcOwner.top;
		fn_MapWindowPoints(pOwner->getManager()->GetPaintWindow(), HWND_DESKTOP, (LPPOINT)&rc, 2);
    }
    
    Create(pOwner->getManager()->GetPaintWindow(), 0, WS_POPUP, WS_EX_TOOLWINDOW, rc);
    // HACK: Don't deselect the parent's caption
    HWND hWndParent = _hWnd;
	while (fn_GetParent(hWndParent) != 0) {
		hWndParent = fn_GetParent(hWndParent);
	}
    fn_ShowWindow(_hWnd, SW_SHOW);
    fn_SendMessageW(hWndParent, WM_NCACTIVATE, TRUE, 0L);
}

const String& CComboWnd::GetWindowClassName() const
{
    return WINDOW_CLASS_NAME;
}

void CComboWnd::OnFinalMessage(HWND hWnd)
{
    m_pOwner->m_pWindow = 0;
    m_pOwner->m_uButtonState &= ~ UISTATE_PUSHED;
    m_pOwner->Invalidate();
    delete this;
}

LRESULT CComboWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE) {
        _paintManager.Init(_hWnd);
        // The trick is to add the items to the new container. Their owner gets
        // reassigned by this operation - which is why it is important to reassign
        // the items back to the righfull owner/manager when the window closes.
        m_pLayout = new VerticalLayout;
        _paintManager.UseParentResource(m_pOwner->getManager());
        m_pLayout->SetManager(&_paintManager, 0, true);
        const String& defaultAttributes = m_pOwner->getManager()->getDefaultAttributeList("VerticalLayout");
		if (!defaultAttributes.isEmpty()) {
			m_pLayout->applyAttributeList(defaultAttributes);
        }
        m_pLayout->setInset(Rect(1, 1, 1, 1));
        m_pLayout->SetBkColor(0xFFFFFFFF);
        m_pLayout->SetBorderColor(0xFFC6C7D2);
        m_pLayout->SetBorderSize(1);
        m_pLayout->SetAutoDestroy(false);
        m_pLayout->enableScrollBar();
        m_pLayout->applyAttributeList(m_pOwner->GetDropBoxAttributeList());
        for (int i = 0; i < m_pOwner->getCount(); ++i) {
            m_pLayout->add(static_cast<Control*>(m_pOwner->getItem(i)));
        }
		_paintManager.attachRootControl(m_pLayout);
        
        return 0;
    }
    else if (uMsg == WM_CLOSE) {
        m_pOwner->SetManager(m_pOwner->getManager(), m_pOwner->GetParent(), false);
        m_pOwner->SetPos(m_pOwner->GetPos());
        m_pOwner->SetFocus();
    }
    else if (uMsg == WM_LBUTTONUP) {
        POINT pt = { 0 };
        fn_GetCursorPos(&pt);
        fn_ScreenToClient(_paintManager.GetPaintWindow(), &pt);
        Control* pControl = _paintManager.FindControl(pt);
        if (pControl && pControl->getClass() != "ScrollBarUI") {
            postMessage(WM_KILLFOCUS);
        }
    }
    else if (uMsg == WM_KEYDOWN) {
        switch( wParam ) {
        case VK_ESCAPE:
            m_pOwner->SelectItem(m_iOldSel, true);
            EnsureVisible(m_iOldSel);
            // FALL THROUGH...
        case VK_RETURN:
            postMessage(WM_KILLFOCUS);
            break;
        default:
            TEventUI event;
            event.Type = UIEVENT_KEYDOWN;
            event.chKey = (TCHAR)wParam;
            m_pOwner->DoEvent(event);
            EnsureVisible(m_pOwner->GetCurSel());
            return 0;
        }
    }
    else if( uMsg == WM_MOUSEWHEEL ) {
        int zDelta = (int) (short) HIWORD(wParam);
        TEventUI event;
		__stosb((uint8_t*)&event, 0, sizeof(event));
        event.Type = UIEVENT_SCROLLWHEEL;
        event.wParam = MAKELPARAM(zDelta < 0 ? SB_LINEDOWN : SB_LINEUP, 0);
        event.lParam = lParam;
        event.dwTimestamp = fn_GetTickCount();
        m_pOwner->DoEvent(event);
        EnsureVisible(m_pOwner->GetCurSel());
        return 0;
    }
    else if (uMsg == WM_KILLFOCUS) {
		if (_hWnd != (HWND)wParam) {
			postMessage(WM_CLOSE);
		}
    }

    LRESULT lRes = 0;
	if (_paintManager.MessageHandler(uMsg, wParam, lParam, lRes)) {
		return lRes;
	}
    return Window::HandleMessage(uMsg, wParam, lParam);
}

void CComboWnd::EnsureVisible(int iIndex)
{
	if (m_pOwner->GetCurSel() < 0) {
		return;
	}
    m_pLayout->FindSelectable(m_pOwner->GetCurSel(), false);
    RECT rcItem = m_pLayout->getItem(iIndex)->GetPos();
    RECT rcList = m_pLayout->GetPos();
    ScrollBar* pHorizontalScrollBar = m_pLayout->getHorizontalScrollBar();
	if (pHorizontalScrollBar && pHorizontalScrollBar->IsVisible()) {
		rcList.bottom -= pHorizontalScrollBar->GetFixedHeight();
	}
    int iPos = m_pLayout->getScrollPos().cy;
	if (rcItem.top >= rcList.top && rcItem.bottom < rcList.bottom) {
		return;
	}
    int dx = 0;
	if (rcItem.top < rcList.top) {
		dx = rcItem.top - rcList.top;
	}
	if (rcItem.bottom > rcList.bottom) {
		dx = rcItem.bottom - rcList.bottom;
	}
    Scroll(0, dx);
}

void CComboWnd::Scroll(int dx, int dy)
{
	if (dx == 0 && dy == 0) {
		return;
	}
    SIZE sz = m_pLayout->getScrollPos();
    m_pLayout->setScrollPos(Size(sz.cx + dx, sz.cy + dy));
}

#if(_WIN32_WINNT >= 0x0501)
UINT CComboWnd::GetClassStyle() const
{
	return __super::GetClassStyle() | CS_DROPSHADOW;
}
#endif
////////////////////////////////////////////////////////

const String Combo::CLASS_NAME = ZGUI_COMBO;

Combo::Combo() :
m_pWindow(0),
m_iCurSel(-1),
m_uButtonState(0)
{
    m_szDropBox = Size(0, 150);
    __stosb((uint8_t*)&m_rcTextPadding, 0, sizeof(m_rcTextPadding));

    m_ListInfo.nColumns = 0;
    m_ListInfo.nFont = -1;
    m_ListInfo.uTextStyle = DT_VCENTER;
    m_ListInfo.dwTextColor = 0xFF000000;
    m_ListInfo.dwBkColor = 0;
    m_ListInfo.bAlternateBk = false;
    m_ListInfo.dwSelectedTextColor = 0xFF000000;
    m_ListInfo.dwSelectedBkColor = 0xFFC1E3FF;
    m_ListInfo.dwHotTextColor = 0xFF000000;
    m_ListInfo.dwHotBkColor = 0xFFE9F5FF;
    m_ListInfo.dwDisabledTextColor = 0xFFCCCCCC;
    m_ListInfo.dwDisabledBkColor = 0xFFFFFFFF;
    m_ListInfo.dwLineColor = 0;
    m_ListInfo.bShowHtml = false;
    m_ListInfo.bMultiExpandable = false;
    __stosb((uint8_t*)&m_ListInfo.rcTextPadding, 0, sizeof(m_ListInfo.rcTextPadding));
	__stosb((uint8_t*)&m_ListInfo.rcColumn, 0, sizeof(m_ListInfo.rcColumn));
}

const String& Combo::getClass() const
{
    return CLASS_NAME;
}

LPVOID Combo::getInterface(const String& name)
{
    if (name == ZGUI_COMBO) {
        return static_cast<Combo*>(this);
    }
    if (name == "IListOwner") {
        return static_cast<IListOwner*>(this);
    }
    return Container::getInterface(name);
}

UINT Combo::GetControlFlags() const
{
    return UIFLAG_TABSTOP;
}

void Combo::DoInit()
{
}

int Combo::GetCurSel() const
{
    return m_iCurSel;
}

bool Combo::SelectItem(int iIndex, bool bTakeFocus)
{
    if (m_pWindow != 0) {
        m_pWindow->Close();
    }
    if (iIndex == m_iCurSel) {
        return true;
    }
    int iOldSel = m_iCurSel;
    if (m_iCurSel >= 0) {
        Control* pControl = static_cast<Control*>(_items[m_iCurSel]);
		if (!pControl) {
			return false;
		}
        IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface(ZGUI_LISTITEM));
		if (pListItem != 0) {
			pListItem->Select(false);
		}
        m_iCurSel = -1;
    }
    if (iIndex < 0 ) {
        return false;
    }
    if (_items.size() == 0 ) {
        return false;
    }
    if (iIndex >= _items.size()) {
        iIndex = _items.size() - 1;
    }
    m_iCurSel = iIndex;
	Control* pControl = static_cast<Control*>(_items[iIndex]);

	IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface(ZGUI_LISTITEM));
	if (pListItem == 0) {
		return false;
	}
	pListItem->Select(true);

	if (!pControl || !pControl->IsVisible() || !pControl->IsEnabled()) {
		return false;
	}
	if (m_pWindow != 0 || bTakeFocus) {
		pControl->SetFocus();
	}
	if (_pManager != 0) {
		_pManager->SendNotify(this, ZGUI_MSGTYPE_ITEMSELECT, m_iCurSel, iOldSel);
	}
    Invalidate();

    return true;
}

bool Combo::setItem(int iIndex, Control* pControl)
{
    int iOrginIndex = indexOf(pControl);
	if (iOrginIndex == -1) {
		return false;
	}
	if (iOrginIndex == iIndex) {
		return true;
	}

    IListItem* pSelectedListItem = 0;
	if (m_iCurSel >= 0) {
		pSelectedListItem = static_cast<IListItem*>(getItem(m_iCurSel)->getInterface(ZGUI_LISTITEM));
	}
	if (!Container::setItem(iIndex, pControl)) {
		return false;
	}
    int iMinIndex = min(iOrginIndex, iIndex);
    int iMaxIndex = max(iOrginIndex, iIndex);
    for (int i = iMinIndex; i < iMaxIndex + 1; ++i) {
        Control* p = getItem(i);
        IListItem* pListItem = static_cast<IListItem*>(p->getInterface(ZGUI_LISTITEM));
        if (pListItem != 0) {
            pListItem->SetIndex(i);
        }
    }
	if (m_iCurSel >= 0 && pSelectedListItem != 0) {
		m_iCurSel = pSelectedListItem->GetIndex();
	}
    return true;
}

bool Combo::add(Control* pControl)
{
    IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface(ZGUI_LISTITEM));
    if (pListItem != 0) {
        pListItem->SetOwner(this);
        pListItem->SetIndex(_items.size());
    }
    return Container::add(pControl);
}

bool Combo::insert(int iIndex, Control* pControl)
{
	if (!Container::insert(iIndex, pControl)) {
		return false;
	}

    // The list items should know about us
    IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface(ZGUI_LISTITEM));
    if (pListItem != 0) {
        pListItem->SetOwner(this);
        pListItem->SetIndex(iIndex);
    }

    for(int i = iIndex + 1; i < getCount(); ++i) {
        Control* p = getItem(i);
        pListItem = static_cast<IListItem*>(p->getInterface(ZGUI_LISTITEM));
        if (pListItem != 0) {
            pListItem->SetIndex(i);
        }
    }
	if (m_iCurSel >= iIndex) {
		m_iCurSel += 1;
	}
    return true;
}

bool Combo::remove(Control* pControl)
{
    int iIndex = indexOf(pControl);
	if (iIndex == -1) {
		return false;
	}

	if (!Container::removeAt(iIndex)) {
		return false;
	}

    for (int i = iIndex; i < getCount(); ++i) {
        Control* p = getItem(i);
        IListItem* pListItem = static_cast<IListItem*>(p->getInterface(ZGUI_LISTITEM));
        if( pListItem != 0 ) {
            pListItem->SetIndex(i);
        }
    }

    if( iIndex == m_iCurSel && m_iCurSel >= 0 ) {
        int iSel = m_iCurSel;
        m_iCurSel = -1;
        SelectItem(FindSelectable(iSel, false));
    }
    else if( iIndex < m_iCurSel ) m_iCurSel -= 1;
    return true;
}

bool Combo::removeAt(int iIndex)
{
	if (!Container::removeAt(iIndex)) {
		return false;
	}

    for (int i = iIndex; i < getCount(); ++i) {
        Control* p = getItem(i);
        IListItem* pListItem = static_cast<IListItem*>(p->getInterface(ZGUI_LISTITEM));
		if (pListItem != 0) {
			pListItem->SetIndex(i);
		}
    }

    if (iIndex == m_iCurSel && m_iCurSel >= 0) {
        int iSel = m_iCurSel;
        m_iCurSel = -1;
        SelectItem(FindSelectable(iSel, false));
    }
	else if (iIndex < m_iCurSel) {
		m_iCurSel -= 1;
	}
    return true;
}

void Combo::removeAll()
{
    m_iCurSel = -1;
    Container::removeAll();
}

void Combo::DoEvent(TEventUI& event)
{
    if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
		if (_pParent != 0) {
			_pParent->DoEvent(event);
		}
		else {
			Container::DoEvent(event);
		}
        return;
    }

    if (event.Type == UIEVENT_SETFOCUS) {
        Invalidate();
    }
    if (event.Type == UIEVENT_KILLFOCUS) {
        Invalidate();
    }
    if (event.Type == UIEVENT_BUTTONDOWN) {
        if (IsEnabled()) {
            Activate();
            m_uButtonState |= UISTATE_PUSHED | UISTATE_CAPTURED;
        }
        return;
    }
    if (event.Type == UIEVENT_BUTTONUP) {
        if ((m_uButtonState & UISTATE_CAPTURED) != 0) {
            m_uButtonState &= ~ UISTATE_CAPTURED;
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_MOUSEMOVE) {
        return;
    }
    if (event.Type == UIEVENT_KEYDOWN) {
        switch (event.chKey) {
        case VK_F4:
            Activate();
            return;
        case VK_UP:
            SelectItem(FindSelectable(m_iCurSel - 1, false));
            return;
        case VK_DOWN:
            SelectItem(FindSelectable(m_iCurSel + 1, true));
            return;
        case VK_PRIOR:
            SelectItem(FindSelectable(m_iCurSel - 1, false));
            return;
        case VK_NEXT:
            SelectItem(FindSelectable(m_iCurSel + 1, true));
            return;
        case VK_HOME:
            SelectItem(FindSelectable(0, false));
            return;
        case VK_END:
            SelectItem(FindSelectable(getCount() - 1, true));
            return;
        }
    }
    if (event.Type == UIEVENT_SCROLLWHEEL) {
		if (m_bEnabled) {
			bool bDownward = LOWORD(event.wParam) == SB_LINEDOWN;
			SelectItem(FindSelectable(m_iCurSel + (bDownward ? 1 : -1), bDownward));
		}
        return;
    }
    if (event.Type == UIEVENT_CONTEXTMENU) {
        return;
    }
    if (event.Type == UIEVENT_MOUSEENTER) {
        if (fn_PtInRect(&_rcItem, event.ptMouse)) {
			if ((m_uButtonState & UISTATE_HOT) == 0) {
				m_uButtonState |= UISTATE_HOT;
			}
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_MOUSELEAVE) {
        if ((m_uButtonState & UISTATE_HOT) != 0) {
            m_uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    Control::DoEvent(event);
}

SIZE Combo::EstimateSize(SIZE szAvailable)
{
	if (_cxyFixed.cy == 0) {
		return Size(_cxyFixed.cx, _pManager->GetDefaultFontInfo()->tm.tmHeight + 12);
	}
    return Control::EstimateSize(szAvailable);
}

bool Combo::Activate()
{
	if (!Control::Activate()) {
		return false;
	}
	if (m_pWindow) {
		return true;
	}
    m_pWindow = new CComboWnd();
    zgui_assert(m_pWindow);
    m_pWindow->Init(this);
	if (_pManager != 0) {
		_pManager->SendNotify(this, ZGUI_MSGTYPE_DROPDOWN);
	}
    Invalidate();
    return true;
}

String Combo::getText() const
{
	if (m_iCurSel < 0) {
		return String::empty;
	}
    Control* pControl = static_cast<Control*>(_items[m_iCurSel]);
    return pControl->getText();
}

void Combo::SetEnabled(bool bEnable)
{
    Container::SetEnabled(bEnable);
	if (!IsEnabled()) {
		m_uButtonState = 0;
	}
}

String Combo::GetDropBoxAttributeList()
{
    return m_sDropBoxAttributes;
}

void Combo::SetDropBoxAttributeList(const String& pstrList)
{
    m_sDropBoxAttributes = pstrList;
}

SIZE Combo::GetDropBoxSize() const
{
    return m_szDropBox;
}

void Combo::SetDropBoxSize(SIZE szDropBox)
{
    m_szDropBox = szDropBox;
}

RECT Combo::GetTextPadding() const
{
    return m_rcTextPadding;
}

void Combo::SetTextPadding(RECT rc)
{
    m_rcTextPadding = rc;
    Invalidate();
}

const String& Combo::GetNormalImage() const
{
    return m_sNormalImage;
}

void Combo::SetNormalImage(const String& pStrImage)
{
    m_sNormalImage = pStrImage;
    Invalidate();
}

const String& Combo::GetHotImage() const
{
    return m_sHotImage;
}

void Combo::SetHotImage(const String& pStrImage)
{
    m_sHotImage = pStrImage;
    Invalidate();
}

const String& Combo::GetPushedImage() const
{
    return m_sPushedImage;
}

void Combo::SetPushedImage(const String& pStrImage)
{
    m_sPushedImage = pStrImage;
    Invalidate();
}

const String& Combo::GetFocusedImage() const
{
    return m_sFocusedImage;
}

void Combo::SetFocusedImage(const String& pStrImage)
{
    m_sFocusedImage = pStrImage;
    Invalidate();
}

const String& Combo::GetDisabledImage() const
{
    return m_sDisabledImage;
}

void Combo::SetDisabledImage(const String& pStrImage)
{
    m_sDisabledImage = pStrImage;
    Invalidate();
}

ListInfo* Combo::GetListInfo()
{
    return &m_ListInfo;
}

void Combo::SetItemFont(int index)
{
    m_ListInfo.nFont = index;
    Invalidate();
}

void Combo::SetItemTextStyle(UINT uStyle)
{
	m_ListInfo.uTextStyle = uStyle;
	Invalidate();
}

RECT Combo::GetItemTextPadding() const
{
	return m_ListInfo.rcTextPadding;
}

void Combo::SetItemTextPadding(RECT rc)
{
    m_ListInfo.rcTextPadding = rc;
    Invalidate();
}

void Combo::SetItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwTextColor = dwTextColor;
    Invalidate();
}

void Combo::SetItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwBkColor = dwBkColor;
}

void Combo::SetItemBkImage(const String& pStrImage)
{
    m_ListInfo.sBkImage = pStrImage;
}

DWORD Combo::GetItemTextColor() const
{
	return m_ListInfo.dwTextColor;
}

DWORD Combo::GetItemBkColor() const
{
	return m_ListInfo.dwBkColor;
}

const String& Combo::GetItemBkImage() const
{
	return m_ListInfo.sBkImage;
}

bool Combo::IsAlternateBk() const
{
    return m_ListInfo.bAlternateBk;
}

void Combo::SetAlternateBk(bool bAlternateBk)
{
    m_ListInfo.bAlternateBk = bAlternateBk;
}

void Combo::SetSelectedItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwSelectedTextColor = dwTextColor;
}

void Combo::SetSelectedItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwSelectedBkColor = dwBkColor;
}

void Combo::SetSelectedItemImage(const String& pStrImage)
{
	m_ListInfo.sSelectedImage = pStrImage;
}

DWORD Combo::GetSelectedItemTextColor() const
{
	return m_ListInfo.dwSelectedTextColor;
}

DWORD Combo::GetSelectedItemBkColor() const
{
	return m_ListInfo.dwSelectedBkColor;
}

const String& Combo::GetSelectedItemImage() const
{
	return m_ListInfo.sSelectedImage;
}

void Combo::SetHotItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwHotTextColor = dwTextColor;
}

void Combo::SetHotItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwHotBkColor = dwBkColor;
}

void Combo::SetHotItemImage(const String& pStrImage)
{
    m_ListInfo.sHotImage = pStrImage;
}

DWORD Combo::GetHotItemTextColor() const
{
	return m_ListInfo.dwHotTextColor;
}
DWORD Combo::GetHotItemBkColor() const
{
	return m_ListInfo.dwHotBkColor;
}

const String& Combo::GetHotItemImage() const
{
	return m_ListInfo.sHotImage;
}

void Combo::SetDisabledItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwDisabledTextColor = dwTextColor;
}

void Combo::SetDisabledItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwDisabledBkColor = dwBkColor;
}

void Combo::SetDisabledItemImage(const String& pStrImage)
{
    m_ListInfo.sDisabledImage = pStrImage;
}

DWORD Combo::GetDisabledItemTextColor() const
{
	return m_ListInfo.dwDisabledTextColor;
}

DWORD Combo::GetDisabledItemBkColor() const
{
	return m_ListInfo.dwDisabledBkColor;
}

const String& Combo::GetDisabledItemImage() const
{
	return m_ListInfo.sDisabledImage;
}

DWORD Combo::GetItemLineColor() const
{
	return m_ListInfo.dwLineColor;
}

void Combo::SetItemLineColor(DWORD dwLineColor)
{
    m_ListInfo.dwLineColor = dwLineColor;
}

bool Combo::IsItemShowHtml()
{
    return m_ListInfo.bShowHtml;
}

void Combo::SetItemShowHtml(bool bShowHtml)
{
    if( m_ListInfo.bShowHtml == bShowHtml ) return;

    m_ListInfo.bShowHtml = bShowHtml;
    Invalidate();
}

void Combo::SetPos(RECT rc)
{
    // Put all elements out of sight
    RECT rcNull;
	__stosb((uint8_t*)&rcNull, 0, sizeof(rcNull));
	for (int i = 0; i < _items.size(); ++i) {
		_items.getUnchecked(i)->SetPos(rcNull);
	}
    // Position this control
    Control::SetPos(rc);
}

void Combo::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "textpadding") {
        RECT rcTextPadding;
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcTextPadding.left, (int&)rcTextPadding.top, (int&)rcTextPadding.right, (int&)rcTextPadding.bottom)) {
            SetTextPadding(rcTextPadding);
        }
    }
    else if (pstrName == "normalimage") {
        SetNormalImage(pstrValue);
    }
    else if (pstrName == "hotimage") {
        SetHotImage(pstrValue);
    }
    else if (pstrName == "pushedimage") {
        SetPushedImage(pstrValue);
    }
    else if (pstrName == "focusedimage") {
        SetFocusedImage(pstrValue);
    }
    else if (pstrName == "disabledimage") {
        SetDisabledImage(pstrValue);
    }
    else if (pstrName == "dropbox") {
        SetDropBoxAttributeList(pstrValue);
    }
	else if (pstrName == "dropboxsize") {
		SIZE szDropBoxSize;
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)szDropBoxSize.cx, (int&)szDropBoxSize.cy)) {
            SetDropBoxSize(szDropBoxSize);
        }
	}
    else if (pstrName == "itemfont") {
        m_ListInfo.nFont = pstrValue.getIntValue();
    }
    else if (pstrName == "itemalign") {
        if (pstrValue.contains("left")) {
            m_ListInfo.uTextStyle &= ~(DT_CENTER | DT_RIGHT);
            m_ListInfo.uTextStyle |= DT_LEFT;
        }
        if (pstrValue.contains("center")) {
            m_ListInfo.uTextStyle &= ~(DT_LEFT | DT_RIGHT);
            m_ListInfo.uTextStyle |= DT_CENTER;
        }
        if (pstrValue.contains("right")) {
            m_ListInfo.uTextStyle &= ~(DT_LEFT | DT_CENTER);
            m_ListInfo.uTextStyle |= DT_RIGHT;
        }
    }
    if (pstrName == "itemtextpadding") {
        RECT rcTextPadding;
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcTextPadding.left, (int&)rcTextPadding.top, (int&)rcTextPadding.right, (int&)rcTextPadding.bottom)) {
            SetItemTextPadding(rcTextPadding);
        }
    }
    else if (pstrName == "itemtextcolor") {
        SetItemTextColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itembkcolor") {
        SetItemBkColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itembkimage") {
        SetItemBkImage(pstrValue);
    }
    else if (pstrName == "itemaltbk") {
        SetAlternateBk(pstrValue == "true");
    }
    else if (pstrName == "itemselectedtextcolor") {
        SetSelectedItemTextColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itemselectedbkcolor") {
        SetSelectedItemBkColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itemselectedimage") {
        SetSelectedItemImage(pstrValue);
    }
    else if (pstrName == "itemhottextcolor") {
        SetHotItemTextColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itemhotbkcolor") {
        SetHotItemBkColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itemhotimage") {
        SetHotItemImage(pstrValue);
    }
    else if (pstrName == "itemdisabledtextcolor") {
        SetDisabledItemTextColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itemdisabledbkcolor") {
        SetDisabledItemBkColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itemdisabledimage") {
        SetDisabledItemImage(pstrValue);
    }
    else if (pstrName == "itemlinecolor") {
        SetItemLineColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "itemshowhtml") {
        SetItemShowHtml(pstrValue == "true");
    }
    else {
        Container::setAttribute(pstrName, pstrValue);
    }
}

void Combo::DoPaint(HDC hDC, const RECT& rcPaint)
{
    Control::DoPaint(hDC, rcPaint);
}

void Combo::PaintStatusImage(HDC hDC)
{
	if (IsFocused()) {
		m_uButtonState |= UISTATE_FOCUSED;
	}
    else m_uButtonState &= ~ UISTATE_FOCUSED;
	if (!IsEnabled()) {
		m_uButtonState |= UISTATE_DISABLED;
	}
    else m_uButtonState &= ~ UISTATE_DISABLED;

    if ((m_uButtonState & UISTATE_DISABLED) != 0) {
        if (!m_sDisabledImage.isEmpty()) {
			if (!DrawImage(hDC, m_sDisabledImage)) {
				m_sDisabledImage = String::empty;
			}
			else {
				return;
			}
        }
    }
    else if ((m_uButtonState & UISTATE_PUSHED) != 0) {
        if (!m_sPushedImage.isEmpty()) {
			if (!DrawImage(hDC, m_sPushedImage)) {
				m_sPushedImage = String::empty;
			}
			else {
				return;
			}
        }
    }
    else if ((m_uButtonState & UISTATE_HOT) != 0) {
        if (!m_sHotImage.isEmpty()) {
			if (!DrawImage(hDC, m_sHotImage)) {
				m_sHotImage = String::empty;
			}
			else {
				return;
			}
        }
    }
    else if ((m_uButtonState & UISTATE_FOCUSED) != 0) {
        if (!m_sFocusedImage.isEmpty()) {
			if (!DrawImage(hDC, m_sFocusedImage)) {
				m_sFocusedImage = String::empty;
			}
			else {
				return;
			}
        }
    }

    if (!m_sNormalImage.isEmpty()) {
		if (!DrawImage(hDC, m_sNormalImage)) {
			m_sNormalImage = String::empty;
		}
		else {
			return;
		}
    }
}

void Combo::PaintText(HDC hDC)
{
    RECT rcText = _rcItem;
    rcText.left += m_rcTextPadding.left;
    rcText.right -= m_rcTextPadding.right;
    rcText.top += m_rcTextPadding.top;
    rcText.bottom -= m_rcTextPadding.bottom;

    if (m_iCurSel >= 0) {
        Control* pControl = static_cast<Control*>(_items[m_iCurSel]);
        IListItem* pElement = static_cast<IListItem*>(pControl->getInterface(ZGUI_LISTITEM));
        if (pElement != 0) {
            pElement->DrawItemText(hDC, rcText);
        }
        else {
            RECT rcOldPos = pControl->GetPos();
            pControl->SetPos(rcText);
            pControl->DoPaint(hDC, rcText);
            pControl->SetPos(rcOldPos);
        }
    }
}

} // namespace zgui

#endif // ZGUI_USE_COMBO