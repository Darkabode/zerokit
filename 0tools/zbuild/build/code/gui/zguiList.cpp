#include "zgui.h"

#ifdef ZGUI_USE_LIST

namespace zgui {

CListUI::CListUI() : m_pCallback(NULL), m_bScrollSelect(false), m_iCurSel(-1), m_iExpandedItem(-1)
{
    m_pList = new CListBodyUI(this);
    m_pHeader = new CListHeaderUI;

    Add(m_pHeader);
    CVerticalLayoutUI::Add(m_pList);

    m_ListInfo.nColumns = 0;
    m_ListInfo.nFont = -1;
    m_ListInfo.uTextStyle = DT_VCENTER; // m_uTextStyle(DT_VCENTER | DT_END_ELLIPSIS)
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
    ::ZeroMemory(&m_ListInfo.rcTextPadding, sizeof(m_ListInfo.rcTextPadding));
    ::ZeroMemory(&m_ListInfo.rcColumn, sizeof(m_ListInfo.rcColumn));
}

LPCTSTR CListUI::GetClass() const
{
    return _T("ListUI");
}

UINT CListUI::GetControlFlags() const
{
    return UIFLAG_TABSTOP;
}

LPVOID CListUI::GetInterface(LPCTSTR pstrName)
{
	if( lstrcmp(pstrName, DUI_CTR_LIST) == 0 ) return static_cast<CListUI*>(this);
    if( lstrcmp(pstrName, _T("IList")) == 0 ) return static_cast<IListUI*>(this);
    if( lstrcmp(pstrName, _T("IListOwner")) == 0 ) return static_cast<IListOwnerUI*>(this);
    return CVerticalLayoutUI::GetInterface(pstrName);
}

CControlUI* CListUI::GetItemAt(int iIndex) const
{
    return m_pList->GetItemAt(iIndex);
}

int CListUI::GetItemIndex(CControlUI* pControl) const
{
    if( pControl->GetInterface(_T("ListHeader")) != NULL ) return CVerticalLayoutUI::GetItemIndex(pControl);
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), _T("ListHeaderItemUI")) != NULL ) return m_pHeader->GetItemIndex(pControl);

    return m_pList->GetItemIndex(pControl);
}

bool CListUI::SetItemIndex(CControlUI* pControl, int iIndex)
{
    if( pControl->GetInterface(_T("ListHeader")) != NULL ) return CVerticalLayoutUI::SetItemIndex(pControl, iIndex);
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), _T("ListHeaderItemUI")) != NULL ) return m_pHeader->SetItemIndex(pControl, iIndex);

    int iOrginIndex = m_pList->GetItemIndex(pControl);
    if( iOrginIndex == -1 ) return false;
    if( iOrginIndex == iIndex ) return true;

    IListItemUI* pSelectedListItem = NULL;
    if( m_iCurSel >= 0 ) pSelectedListItem = 
        static_cast<IListItemUI*>(GetItemAt(m_iCurSel)->GetInterface(_T("ListItem")));
    if( !m_pList->SetItemIndex(pControl, iIndex) ) return false;
    int iMinIndex = min(iOrginIndex, iIndex);
    int iMaxIndex = max(iOrginIndex, iIndex);
    for(int i = iMinIndex; i < iMaxIndex + 1; ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(_T("ListItem")));
        if( pListItem != NULL ) {
            pListItem->SetIndex(i);
        }
    }
    if( m_iCurSel >= 0 && pSelectedListItem != NULL ) m_iCurSel = pSelectedListItem->GetIndex();
    return true;
}

int CListUI::GetCount() const
{
    return m_pList->GetCount();
}

bool CListUI::Add(CControlUI* pControl)
{
    // Override the Add() method so we can add items specifically to
    // the intended widgets. Headers are assumed to be
    // answer the correct interface so we can add multiple list headers.
    if( pControl->GetInterface(_T("ListHeader")) != NULL ) {
        if( m_pHeader != pControl && m_pHeader->GetCount() == 0 ) {
            CVerticalLayoutUI::Remove(m_pHeader);
            m_pHeader = static_cast<CListHeaderUI*>(pControl);
        }
        m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
        return CVerticalLayoutUI::AddAt(pControl, 0);
    }
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), _T("ListHeaderItemUI")) != NULL ) {
        bool ret = m_pHeader->Add(pControl);
        m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
        return ret;
    }
    // The list items should know about us
    IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(_T("ListItem")));
    if( pListItem != NULL ) {
        pListItem->SetOwner(this);
        pListItem->SetIndex(GetCount());
    }
    return m_pList->Add(pControl);
}

bool CListUI::AddAt(CControlUI* pControl, int iIndex)
{
    // Override the AddAt() method so we can add items specifically to
    // the intended widgets. Headers and are assumed to be
    // answer the correct interface so we can add multiple list headers.
    if( pControl->GetInterface(_T("ListHeader")) != NULL ) {
        if( m_pHeader != pControl && m_pHeader->GetCount() == 0 ) {
            CVerticalLayoutUI::Remove(m_pHeader);
            m_pHeader = static_cast<CListHeaderUI*>(pControl);
        }
        m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
        return CVerticalLayoutUI::AddAt(pControl, 0);
    }
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), _T("ListHeaderItemUI")) != NULL ) {
        bool ret = m_pHeader->AddAt(pControl, iIndex);
        m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
        return ret;
    }
    if (!m_pList->AddAt(pControl, iIndex)) return false;

    // The list items should know about us
    IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(_T("ListItem")));
    if( pListItem != NULL ) {
        pListItem->SetOwner(this);
        pListItem->SetIndex(iIndex);
    }

    for(int i = iIndex + 1; i < m_pList->GetCount(); ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        pListItem = static_cast<IListItemUI*>(p->GetInterface(_T("ListItem")));
        if( pListItem != NULL ) {
            pListItem->SetIndex(i);
        }
    }
    if( m_iCurSel >= iIndex ) m_iCurSel += 1;
    return true;
}

bool CListUI::Remove(CControlUI* pControl)
{
    if( pControl->GetInterface(_T("ListHeader")) != NULL ) return CVerticalLayoutUI::Remove(pControl);
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), _T("ListHeaderItemUI")) != NULL ) return m_pHeader->Remove(pControl);

    int iIndex = m_pList->GetItemIndex(pControl);
    if (iIndex == -1) return false;

    if (!m_pList->RemoveAt(iIndex)) return false;

    for(int i = iIndex; i < m_pList->GetCount(); ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(_T("ListItem")));
        if( pListItem != NULL ) {
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

bool CListUI::RemoveAt(int iIndex)
{
    if (!m_pList->RemoveAt(iIndex)) return false;

    for(int i = iIndex; i < m_pList->GetCount(); ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(_T("ListItem")));
        if( pListItem != NULL ) pListItem->SetIndex(i);
    }

    if( iIndex == m_iCurSel && m_iCurSel >= 0 ) {
        int iSel = m_iCurSel;
        m_iCurSel = -1;
        SelectItem(FindSelectable(iSel, false));
    }
    else if( iIndex < m_iCurSel ) m_iCurSel -= 1;
    return true;
}

void CListUI::RemoveAll()
{
    m_iCurSel = -1;
    m_iExpandedItem = -1;
    m_pList->RemoveAll();
}

void CListUI::SetPos(RECT rc)
{
    CVerticalLayoutUI::SetPos(rc);
    if( m_pHeader == NULL ) return;
    // Determine general list information and the size of header columns
    m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
    // The header/columns may or may not be visible at runtime. In either case
    // we should determine the correct dimensions...

    if( !m_pHeader->IsVisible() ) {
        for( int it = 0; it < m_pHeader->GetCount(); it++ ) {
            static_cast<CControlUI*>(m_pHeader->GetItemAt(it))->SetInternVisible(true);
        }
        m_pHeader->SetPos(CDuiRect(rc.left, 0, rc.right, 0));
    }
    int iOffset = m_pList->GetScrollPos().cx;
    for( int i = 0; i < m_ListInfo.nColumns; i++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_pHeader->GetItemAt(i));
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) continue;

        RECT rcPos = pControl->GetPos();
        if( iOffset > 0 ) {
            rcPos.left -= iOffset;
            rcPos.right -= iOffset;
            pControl->SetPos(rcPos);
        }
        m_ListInfo.rcColumn[i] = pControl->GetPos();
    }
    if( !m_pHeader->IsVisible() ) {
        for( int it = 0; it < m_pHeader->GetCount(); it++ ) {
            static_cast<CControlUI*>(m_pHeader->GetItemAt(it))->SetInternVisible(false);
        }
    }
}

void CListUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( _pParent != NULL ) _pParent->DoEvent(event);
        else CVerticalLayoutUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_SETFOCUS ) 
    {
        m_bFocused = true;
        return;
    }
    if( event.Type == UIEVENT_KILLFOCUS ) 
    {
        m_bFocused = false;
        return;
    }

    switch( event.Type ) {
    case UIEVENT_KEYDOWN:
        switch( event.chKey ) {
        case VK_UP:
            SelectItem(FindSelectable(m_iCurSel - 1, false), true);
            return;
        case VK_DOWN:
            SelectItem(FindSelectable(m_iCurSel + 1, true), true);
            return;
        case VK_PRIOR:
            PageUp();
            return;
        case VK_NEXT:
            PageDown();
            return;
        case VK_HOME:
            SelectItem(FindSelectable(0, false), true);
            return;
        case VK_END:
            SelectItem(FindSelectable(GetCount() - 1, true), true);
            return;
        case VK_RETURN:
            if( m_iCurSel != -1 ) GetItemAt(m_iCurSel)->Activate();
            return;
            }
        break;
    case UIEVENT_SCROLLWHEEL:
        {
            switch( LOWORD(event.wParam) ) {
            case SB_LINEUP:
                if( m_bScrollSelect ) SelectItem(FindSelectable(m_iCurSel - 1, false), true);
                else LineUp();
                return;
            case SB_LINEDOWN:
                if( m_bScrollSelect ) SelectItem(FindSelectable(m_iCurSel + 1, true), true);
                else LineDown();
                return;
            }
        }
        break;
    }
    CVerticalLayoutUI::DoEvent(event);
}

CListHeaderUI* CListUI::GetHeader() const
{
    return m_pHeader;
}

CContainerUI* CListUI::GetList() const
{
    return m_pList;
}

bool CListUI::GetScrollSelect()
{
    return m_bScrollSelect;
}

void CListUI::SetScrollSelect(bool bScrollSelect)
{
    m_bScrollSelect = bScrollSelect;
}

int CListUI::GetCurSel() const
{
    return m_iCurSel;
}

bool CListUI::SelectItem(int iIndex, bool bTakeFocus)
{
    if( iIndex == m_iCurSel ) return true;

    int iOldSel = m_iCurSel;
    // We should first unselect the currently selected item
    if( m_iCurSel >= 0 ) {
        CControlUI* pControl = GetItemAt(m_iCurSel);
        if( pControl != NULL) {
            IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(_T("ListItem")));
            if( pListItem != NULL ) pListItem->Select(false);
        }

        m_iCurSel = -1;
    }
    if( iIndex < 0 ) return false;

    CControlUI* pControl = GetItemAt(iIndex);
    if( pControl == NULL ) return false;
    if( !pControl->IsVisible() ) return false;
    if( !pControl->IsEnabled() ) return false;

    IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(_T("ListItem")));
    if( pListItem == NULL ) return false;
    m_iCurSel = iIndex;
    if( !pListItem->Select(true) ) {
        m_iCurSel = -1;
        return false;
    }
    EnsureVisible(m_iCurSel);
    if( bTakeFocus ) pControl->SetFocus();
    if( _pManager != NULL ) {
        _pManager->SendNotify(this, DUI_MSGTYPE_ITEMSELECT, m_iCurSel, iOldSel);
    }

    return true;
}

TListInfoUI* CListUI::GetListInfo()
{
    return &m_ListInfo;
}

int CListUI::GetChildPadding() const
{
    return m_pList->GetChildPadding();
}

void CListUI::SetChildPadding(int iPadding)
{
    m_pList->SetChildPadding(iPadding);
}

void CListUI::SetItemFont(int index)
{
    m_ListInfo.nFont = index;
    NeedUpdate();
}

void CListUI::SetItemTextStyle(UINT uStyle)
{
    m_ListInfo.uTextStyle = uStyle;
    NeedUpdate();
}

void CListUI::SetItemTextPadding(RECT rc)
{
    m_ListInfo.rcTextPadding = rc;
    NeedUpdate();
}

RECT CListUI::GetItemTextPadding() const
{
	return m_ListInfo.rcTextPadding;
}

void CListUI::SetItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetItemBkImage(const String& pStrImage)
{
    m_ListInfo.sBkImage = pStrImage;
    Invalidate();
}

void CListUI::SetAlternateBk(bool bAlternateBk)
{
    m_ListInfo.bAlternateBk = bAlternateBk;
    Invalidate();
}

DWORD CListUI::GetItemTextColor() const
{
	return m_ListInfo.dwTextColor;
}

DWORD CListUI::GetItemBkColor() const
{
	return m_ListInfo.dwBkColor;
}

const String& CListUI::GetItemBkImage() const
{
	return m_ListInfo.sBkImage;
}

bool CListUI::IsAlternateBk() const
{
    return m_ListInfo.bAlternateBk;
}

void CListUI::SetSelectedItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwSelectedTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetSelectedItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwSelectedBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetSelectedItemImage(const String& pStrImage)
{
    m_ListInfo.sSelectedImage = pStrImage;
    Invalidate();
}

DWORD CListUI::GetSelectedItemTextColor() const
{
	return m_ListInfo.dwSelectedTextColor;
}

DWORD CListUI::GetSelectedItemBkColor() const
{
	return m_ListInfo.dwSelectedBkColor;
}

const String& CListUI::GetSelectedItemImage() const
{
	return m_ListInfo.sSelectedImage;
}

void CListUI::SetHotItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwHotTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetHotItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwHotBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetHotItemImage(const String& pStrImage)
{
    m_ListInfo.sHotImage = pStrImage;
    Invalidate();
}

DWORD CListUI::GetHotItemTextColor() const
{
	return m_ListInfo.dwHotTextColor;
}
DWORD CListUI::GetHotItemBkColor() const
{
	return m_ListInfo.dwHotBkColor;
}

const String& CListUI::GetHotItemImage() const
{
	return m_ListInfo.sHotImage;
}

void CListUI::SetDisabledItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwDisabledTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetDisabledItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwDisabledBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetDisabledItemImage(const String& pStrImage)
{
    m_ListInfo.sDisabledImage = pStrImage;
    Invalidate();
}

DWORD CListUI::GetDisabledItemTextColor() const
{
	return m_ListInfo.dwDisabledTextColor;
}

DWORD CListUI::GetDisabledItemBkColor() const
{
	return m_ListInfo.dwDisabledBkColor;
}

const String& CListUI::GetDisabledItemImage() const
{
	return m_ListInfo.sDisabledImage;
}

DWORD CListUI::GetItemLineColor() const
{
	return m_ListInfo.dwLineColor;
}

void CListUI::SetItemLineColor(DWORD dwLineColor)
{
    m_ListInfo.dwLineColor = dwLineColor;
    Invalidate();
}

bool CListUI::IsItemShowHtml()
{
    return m_ListInfo.bShowHtml;
}

void CListUI::SetItemShowHtml(bool bShowHtml)
{
    if( m_ListInfo.bShowHtml == bShowHtml ) return;

    m_ListInfo.bShowHtml = bShowHtml;
    NeedUpdate();
}

void CListUI::SetMultiExpanding(bool bMultiExpandable)
{
    m_ListInfo.bMultiExpandable = bMultiExpandable;
}

bool CListUI::ExpandItem(int iIndex, bool bExpand /*= true*/)
{
    if( m_iExpandedItem >= 0 && !m_ListInfo.bMultiExpandable) {
        CControlUI* pControl = GetItemAt(m_iExpandedItem);
        if( pControl != NULL ) {
            IListItemUI* pItem = static_cast<IListItemUI*>(pControl->GetInterface(_T("ListItem")));
            if( pItem != NULL ) pItem->Expand(false);
        }
        m_iExpandedItem = -1;
    }
    if( bExpand ) {
        CControlUI* pControl = GetItemAt(iIndex);
        if( pControl == NULL ) return false;
        if( !pControl->IsVisible() ) return false;
        IListItemUI* pItem = static_cast<IListItemUI*>(pControl->GetInterface(_T("ListItem")));
        if( pItem == NULL ) return false;
        m_iExpandedItem = iIndex;
        if( !pItem->Expand(true) ) {
            m_iExpandedItem = -1;
            return false;
        }
    }
    NeedUpdate();
    return true;
}

int CListUI::GetExpandedItem() const
{
    return m_iExpandedItem;
}

void CListUI::EnsureVisible(int iIndex)
{
    if( m_iCurSel < 0 ) return;
    RECT rcItem = m_pList->GetItemAt(iIndex)->GetPos();
    RECT rcList = m_pList->GetPos();
    RECT rcListInset = m_pList->GetInset();

    rcList.left += rcListInset.left;
    rcList.top += rcListInset.top;
    rcList.right -= rcListInset.right;
    rcList.bottom -= rcListInset.bottom;

    CScrollBarUI* pHorizontalScrollBar = m_pList->GetHorizontalScrollBar();
    if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rcList.bottom -= pHorizontalScrollBar->GetFixedHeight();

    int iPos = m_pList->GetScrollPos().cy;
    if( rcItem.top >= rcList.top && rcItem.bottom < rcList.bottom ) return;
    int dx = 0;
    if( rcItem.top < rcList.top ) dx = rcItem.top - rcList.top;
    if( rcItem.bottom > rcList.bottom ) dx = rcItem.bottom - rcList.bottom;
    Scroll(0, dx);
}

void CListUI::Scroll(int dx, int dy)
{
    if( dx == 0 && dy == 0 ) return;
    SIZE sz = m_pList->GetScrollPos();
    m_pList->SetScrollPos(CSize(sz.cx + dx, sz.cy + dy));
}

void CListUI::SetAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "header") {
        GetHeader()->SetVisible(pstrValue == "hidden");
    }
    else if (pstrName == "headerbkimage") {
        GetHeader()->SetBkImage(pstrValue);
    }
    else if (pstrName == "scrollselect") {
        SetScrollSelect(pstrValue == "true");
    }
    else if (pstrName == "multiexpanding") {
        SetMultiExpanding(pstrValue == "true");
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
    else if (pstrName == "itemendellipsis") {
        if (pstrValue == "true") {
            m_ListInfo.uTextStyle |= DT_END_ELLIPSIS;
        }
        else {
            m_ListInfo.uTextStyle &= ~DT_END_ELLIPSIS;
        }
    }    
    if (pstrName == "itemtextpadding") {
        RECT rcTextPadding = { 0 };
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
        CVerticalLayoutUI::SetAttribute(pstrName, pstrValue);
    }
}

IListCallbackUI* CListUI::GetTextCallback() const
{
    return m_pCallback;
}

void CListUI::SetTextCallback(IListCallbackUI* pCallback)
{
    m_pCallback = pCallback;
}

SIZE CListUI::GetScrollPos() const
{
    return m_pList->GetScrollPos();
}

SIZE CListUI::GetScrollRange() const
{
    return m_pList->GetScrollRange();
}

void CListUI::SetScrollPos(SIZE szPos)
{
    m_pList->SetScrollPos(szPos);
}

void CListUI::LineUp()
{
    m_pList->LineUp();
}

void CListUI::LineDown()
{
    m_pList->LineDown();
}

void CListUI::PageUp()
{
    m_pList->PageUp();
}

void CListUI::PageDown()
{
    m_pList->PageDown();
}

void CListUI::HomeUp()
{
    m_pList->HomeUp();
}

void CListUI::EndDown()
{
    m_pList->EndDown();
}

void CListUI::LineLeft()
{
    m_pList->LineLeft();
}

void CListUI::LineRight()
{
    m_pList->LineRight();
}

void CListUI::PageLeft()
{
    m_pList->PageLeft();
}

void CListUI::PageRight()
{
    m_pList->PageRight();
}

void CListUI::HomeLeft()
{
    m_pList->HomeLeft();
}

void CListUI::EndRight()
{
    m_pList->EndRight();
}

void CListUI::EnableScrollBar(bool bEnableVertical, bool bEnableHorizontal)
{
    m_pList->EnableScrollBar(bEnableVertical, bEnableHorizontal);
}

CScrollBarUI* CListUI::GetVerticalScrollBar() const
{
    return m_pList->GetVerticalScrollBar();
}

CScrollBarUI* CListUI::GetHorizontalScrollBar() const
{
    return m_pList->GetHorizontalScrollBar();
}

/////////////////////////////////////////////////////////////////////////////////////
//
//


CListBodyUI::CListBodyUI(CListUI* pOwner) : m_pOwner(pOwner)
{
    ASSERT(m_pOwner);
}

void CListBodyUI::SetScrollPos(SIZE szPos)
{
    int cx = 0;
    int cy = 0;
    if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) {
        int iLastScrollPos = _pVerticalScrollBar->GetScrollPos();
        _pVerticalScrollBar->SetScrollPos(szPos.cy);
        cy = _pVerticalScrollBar->GetScrollPos() - iLastScrollPos;
    }

    if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) {
        int iLastScrollPos = _pHorizontalScrollBar->GetScrollPos();
        _pHorizontalScrollBar->SetScrollPos(szPos.cx);
        cx = _pHorizontalScrollBar->GetScrollPos() - iLastScrollPos;
    }

    if( cx == 0 && cy == 0 ) return;

    RECT rcPos;
    for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) continue;

        rcPos = pControl->GetPos();
        rcPos.left -= cx;
        rcPos.right -= cx;
        rcPos.top -= cy;
        rcPos.bottom -= cy;
        pControl->SetPos(rcPos);
    }

    Invalidate();

    if( cx != 0 && m_pOwner ) {
        CListHeaderUI* pHeader = m_pOwner->GetHeader();
        if( pHeader == NULL ) return;
        TListInfoUI* pInfo = m_pOwner->GetListInfo();
        pInfo->nColumns = MIN(pHeader->GetCount(), UILIST_MAX_COLUMNS);

        if( !pHeader->IsVisible() ) {
            for( int it = 0; it < pHeader->GetCount(); it++ ) {
                static_cast<CControlUI*>(pHeader->GetItemAt(it))->SetInternVisible(true);
            }
        }
        for( int i = 0; i < pInfo->nColumns; i++ ) {
            CControlUI* pControl = static_cast<CControlUI*>(pHeader->GetItemAt(i));
            if( !pControl->IsVisible() ) continue;
            if( pControl->IsFloat() ) continue;

            RECT rcPos = pControl->GetPos();
            rcPos.left -= cx;
            rcPos.right -= cx;
            pControl->SetPos(rcPos);
            pInfo->rcColumn[i] = pControl->GetPos();
        }
        if( !pHeader->IsVisible() ) {
            for( int it = 0; it < pHeader->GetCount(); it++ ) {
                static_cast<CControlUI*>(pHeader->GetItemAt(it))->SetInternVisible(false);
            }
        }
    }
}

void CListBodyUI::SetPos(RECT rc)
{
    CControlUI::SetPos(rc);
    rc = _rcItem;

    // Adjust for inset
    rc.left += _rcInset.left;
    rc.top += _rcInset.top;
    rc.right -= _rcInset.right;
    rc.bottom -= _rcInset.bottom;
    if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) rc.right -= _pVerticalScrollBar->GetFixedWidth();
    if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();

    // Determine the minimum size
    SIZE szAvailable = { rc.right - rc.left, rc.bottom - rc.top };
    if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) 
        szAvailable.cx += _pHorizontalScrollBar->GetScrollRange();

    int cxNeeded = 0;
    int nAdjustables = 0;
    int cyFixed = 0;
    int nEstimateNum = 0;
    for( int it1 = 0; it1 < m_items.GetSize(); it1++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it1]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) continue;
        SIZE sz = pControl->EstimateSize(szAvailable);
        if( sz.cy == 0 ) {
            nAdjustables++;
        }
        else {
            if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
            if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();
        }
        cyFixed += sz.cy + pControl->GetPadding().top + pControl->GetPadding().bottom;

        RECT rcPadding = pControl->GetPadding();
        sz.cx = MAX(sz.cx, 0);
        if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
        if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();
        cxNeeded = MAX(cxNeeded, sz.cx);
        nEstimateNum++;
    }
    cyFixed += (nEstimateNum - 1) * m_iChildPadding;

    if( m_pOwner ) {
        CListHeaderUI* pHeader = m_pOwner->GetHeader();
        if( pHeader != NULL && pHeader->GetCount() > 0 ) {
            cxNeeded = MAX(0, pHeader->EstimateSize(CSize(rc.right - rc.left, rc.bottom - rc.top)).cx);
        }
    }

    // Place elements
    int cyNeeded = 0;
    int cyExpand = 0;
    if( nAdjustables > 0 ) cyExpand = MAX(0, (szAvailable.cy - cyFixed) / nAdjustables);
    // Position the elements
    SIZE szRemaining = szAvailable;
    int iPosY = rc.top;
    if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) {
        iPosY -= _pVerticalScrollBar->GetScrollPos();
    }
    int iPosX = rc.left;
    if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) {
        iPosX -= _pHorizontalScrollBar->GetScrollPos();
    }
    int iAdjustable = 0;
    int cyFixedRemaining = cyFixed;
    for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) {
            SetFloatPos(it2);
            continue;
        }

        RECT rcPadding = pControl->GetPadding();
        szRemaining.cy -= rcPadding.top;
        SIZE sz = pControl->EstimateSize(szRemaining);
        if( sz.cy == 0 ) {
            iAdjustable++;
            sz.cy = cyExpand;
            // Distribute remaining to last element (usually round-off left-overs)
            if( iAdjustable == nAdjustables ) {
                sz.cy = MAX(0, szRemaining.cy - rcPadding.bottom - cyFixedRemaining);
            } 
            if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
            if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();
        }
        else {
            if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
            if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();
            cyFixedRemaining -= sz.cy;
        }

        sz.cx = MAX(cxNeeded, szAvailable.cx - rcPadding.left - rcPadding.right);

        if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
        if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();

        RECT rcCtrl = { iPosX + rcPadding.left, iPosY + rcPadding.top, iPosX + rcPadding.left + sz.cx, iPosY + sz.cy + rcPadding.top + rcPadding.bottom };
        pControl->SetPos(rcCtrl);

        iPosY += sz.cy + m_iChildPadding + rcPadding.top + rcPadding.bottom;
        cyNeeded += sz.cy + rcPadding.top + rcPadding.bottom;
        szRemaining.cy -= sz.cy + m_iChildPadding + rcPadding.bottom;
    }
    cyNeeded += (nEstimateNum - 1) * m_iChildPadding;

    if( _pHorizontalScrollBar != NULL ) {
        if( cxNeeded > rc.right - rc.left ) {
            if( _pHorizontalScrollBar->IsVisible() ) {
                _pHorizontalScrollBar->SetScrollRange(cxNeeded - (rc.right - rc.left));
            }
            else {
                _pHorizontalScrollBar->SetVisible(true);
                _pHorizontalScrollBar->SetScrollRange(cxNeeded - (rc.right - rc.left));
                _pHorizontalScrollBar->SetScrollPos(0);
                rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();
            }
        }
        else {
            if( _pHorizontalScrollBar->IsVisible() ) {
                _pHorizontalScrollBar->SetVisible(false);
                _pHorizontalScrollBar->SetScrollRange(0);
                _pHorizontalScrollBar->SetScrollPos(0);
                rc.bottom += _pHorizontalScrollBar->GetFixedHeight();
            }
        }
    }

    // Process the scrollbar
    ProcessScrollBar(rc, cxNeeded, cyNeeded);
}

void CListBodyUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CControlUI::DoEvent(event);
        return;
    }

    if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListHeaderUI::CListHeaderUI()
{
}

LPCTSTR CListHeaderUI::GetClass() const
{
    return _T("ListHeaderUI");
}

LPVOID CListHeaderUI::GetInterface(LPCTSTR pstrName)
{
    if( lstrcmp(pstrName, DUI_CTR_LISTHEADER) == 0 ) return this;
    return CHorizontalLayoutUI::GetInterface(pstrName);
}

SIZE CListHeaderUI::EstimateSize(SIZE szAvailable)
{
    SIZE cXY = {0, _cxyFixed.cy};
	if( cXY.cy == 0 && _pManager != NULL ) {
		for( int it = 0; it < m_items.GetSize(); it++ ) {
			cXY.cy = MAX(cXY.cy,static_cast<CControlUI*>(m_items[it])->EstimateSize(szAvailable).cy);
		}
		int nMin = _pManager->GetDefaultFontInfo()->tm.tmHeight + 6;
		cXY.cy = MAX(cXY.cy,nMin);
	}

    for( int it = 0; it < m_items.GetSize(); it++ ) {
        cXY.cx +=  static_cast<CControlUI*>(m_items[it])->EstimateSize(szAvailable).cx;
    }

    return cXY;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListHeaderItemUI::CListHeaderItemUI() : m_bDragable(true), m_uButtonState(0), m_iSepWidth(4),
m_uTextStyle(DT_VCENTER | DT_CENTER | DT_SINGLELINE), m_dwTextColor(0), m_iFont(-1), m_bShowHtml(false)
{
	SetTextPadding(CDuiRect(2, 0, 2, 0));
    ptLastMouse.x = ptLastMouse.y = 0;
    SetMinWidth(16);
}

LPCTSTR CListHeaderItemUI::GetClass() const
{
    return _T("ListHeaderItemUI");
}

LPVOID CListHeaderItemUI::GetInterface(LPCTSTR pstrName)
{
    if( lstrcmp(pstrName, DUI_CTR_LISTHEADERITEM) == 0 ) return this;
    return CControlUI::GetInterface(pstrName);
}

UINT CListHeaderItemUI::GetControlFlags() const
{
    if( IsEnabled() && m_iSepWidth != 0 ) return UIFLAG_SETCURSOR;
    else return 0;
}

void CListHeaderItemUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

bool CListHeaderItemUI::IsDragable() const
{
	return m_bDragable;
}

void CListHeaderItemUI::SetDragable(bool bDragable)
{
    m_bDragable = bDragable;
    if ( !m_bDragable ) m_uButtonState &= ~UISTATE_CAPTURED;
}

DWORD CListHeaderItemUI::GetSepWidth() const
{
	return m_iSepWidth;
}

void CListHeaderItemUI::SetSepWidth(int iWidth)
{
    m_iSepWidth = iWidth;
}

DWORD CListHeaderItemUI::GetTextStyle() const
{
	return m_uTextStyle;
}

void CListHeaderItemUI::SetTextStyle(UINT uStyle)
{
    m_uTextStyle = uStyle;
    Invalidate();
}

DWORD CListHeaderItemUI::GetTextColor() const
{
	return m_dwTextColor;
}


void CListHeaderItemUI::SetTextColor(DWORD dwTextColor)
{
    m_dwTextColor = dwTextColor;
}

RECT CListHeaderItemUI::GetTextPadding() const
{
	return m_rcTextPadding;
}

void CListHeaderItemUI::SetTextPadding(RECT rc)
{
	m_rcTextPadding = rc;
	Invalidate();
}

void CListHeaderItemUI::SetFont(int index)
{
    m_iFont = index;
}

bool CListHeaderItemUI::IsShowHtml()
{
    return m_bShowHtml;
}

void CListHeaderItemUI::SetShowHtml(bool bShowHtml)
{
    if( m_bShowHtml == bShowHtml ) return;

    m_bShowHtml = bShowHtml;
    Invalidate();
}

const String& CListHeaderItemUI::GetNormalImage() const
{
	return m_sNormalImage;
}

void CListHeaderItemUI::SetNormalImage(const String& pStrImage)
{
    m_sNormalImage = pStrImage;
    Invalidate();
}

const String& CListHeaderItemUI::GetHotImage() const
{
    return m_sHotImage;
}

void CListHeaderItemUI::SetHotImage(const String& pStrImage)
{
    m_sHotImage = pStrImage;
    Invalidate();
}

const String& CListHeaderItemUI::GetPushedImage() const
{
    return m_sPushedImage;
}

void CListHeaderItemUI::SetPushedImage(const String& pStrImage)
{
    m_sPushedImage = pStrImage;
    Invalidate();
}

const String& CListHeaderItemUI::GetFocusedImage() const
{
    return m_sFocusedImage;
}

void CListHeaderItemUI::SetFocusedImage(const String& pStrImage)
{
    m_sFocusedImage = pStrImage;
    Invalidate();
}

const String& CListHeaderItemUI::GetSepImage() const
{
    return m_sSepImage;
}

void CListHeaderItemUI::SetSepImage(const String& pStrImage)
{
    m_sSepImage = pStrImage;
    Invalidate();
}

void CListHeaderItemUI::SetAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "dragable") {
        SetDragable(pstrValue == "true");
    }
    else if (pstrName == "sepwidth") {
        SetSepWidth(pstrValue.getIntValue());
    }
    else if (pstrName == "align") {
        if (pstrValue.contains("left")) {
            m_uTextStyle &= ~(DT_CENTER | DT_RIGHT);
            m_uTextStyle |= DT_LEFT;
        }
        if (pstrValue.contains("center")) {
            m_uTextStyle &= ~(DT_LEFT | DT_RIGHT);
            m_uTextStyle |= DT_CENTER;
        }
        if (pstrValue.contains("right")) {
            m_uTextStyle &= ~(DT_LEFT | DT_CENTER);
            m_uTextStyle |= DT_RIGHT;
        }
    }
    else if (pstrName == "endellipsis") {
        if (pstrValue == "true") {
            m_uTextStyle |= DT_END_ELLIPSIS;
        }
        else {
            m_uTextStyle &= ~DT_END_ELLIPSIS;
        }
    }    
    else if (pstrName == "font") {
        SetFont(pstrValue.getIntValue());
    }
    else if (pstrName == "textcolor") {
        SetTextColor((uint32_t)pstrValue.getHexValue32());
    }
	else if (pstrName == "textpadding") {
		RECT rcTextPadding = { 0 };
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcTextPadding.left, (int&)rcTextPadding.top, (int&)rcTextPadding.right, (int&)rcTextPadding.bottom)) {
            SetTextPadding(rcTextPadding);
        }
	}
    else if (pstrName == "showhtml") {
        SetShowHtml(pstrValue == "true");
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
    else if (pstrName == "sepimage") {
        SetSepImage(pstrValue);
    }
    else {
        CControlUI::SetAttribute(pstrName, pstrValue);
    }
}

void CListHeaderItemUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( _pParent != NULL ) _pParent->DoEvent(event);
        else CControlUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_SETFOCUS ) 
    {
        Invalidate();
    }
    if( event.Type == UIEVENT_KILLFOCUS ) 
    {
        Invalidate();
    }
    if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK )
    {
        if( !IsEnabled() ) return;
        RECT rcSeparator = GetThumbRect();
		if (m_iSepWidth>=0)//111024 by cddjr, 增加分隔符区域，方便用户拖动
			rcSeparator.left-=4;
		else
			rcSeparator.right+=4;
        if( ::PtInRect(&rcSeparator, event.ptMouse) ) {
            if( m_bDragable ) {
                m_uButtonState |= UISTATE_CAPTURED;
                ptLastMouse = event.ptMouse;
            }
        }
        else {
            m_uButtonState |= UISTATE_PUSHED;
            _pManager->SendNotify(this, DUI_MSGTYPE_HEADERCLICK);
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_BUTTONUP )
    {
        if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
            m_uButtonState &= ~UISTATE_CAPTURED;
            if( GetParent() ) 
                GetParent()->NeedParentUpdate();
        }
        else if( (m_uButtonState & UISTATE_PUSHED) != 0 ) {
            m_uButtonState &= ~UISTATE_PUSHED;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE )
    {
        if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
            RECT rc = _rcItem;
            if( m_iSepWidth >= 0 ) {
                rc.right -= ptLastMouse.x - event.ptMouse.x;
            }
            else {
                rc.left -= ptLastMouse.x - event.ptMouse.x;
            }
            
            if( rc.right - rc.left > GetMinWidth() ) {
                _cxyFixed.cx = rc.right - rc.left;
                ptLastMouse = event.ptMouse;
                if( GetParent() ) 
                    GetParent()->NeedParentUpdate();
            }
        }
        return;
    }
    if( event.Type == UIEVENT_SETCURSOR )
    {
        RECT rcSeparator = GetThumbRect();
		if (m_iSepWidth>=0)//111024 by cddjr, 增加分隔符区域，方便用户拖动
			rcSeparator.left-=4;
		else
			rcSeparator.right+=4;
        if( IsEnabled() && m_bDragable && ::PtInRect(&rcSeparator, event.ptMouse) ) {
            ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE)));
            return;
        }
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( IsEnabled() ) {
            m_uButtonState |= UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( IsEnabled() ) {
            m_uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    CControlUI::DoEvent(event);
}

SIZE CListHeaderItemUI::EstimateSize(SIZE szAvailable)
{
    if( _cxyFixed.cy == 0 ) return CSize(_cxyFixed.cx, _pManager->GetDefaultFontInfo()->tm.tmHeight + 14);
    return CControlUI::EstimateSize(szAvailable);
}

RECT CListHeaderItemUI::GetThumbRect() const
{
    if( m_iSepWidth >= 0 ) return CDuiRect(_rcItem.right - m_iSepWidth, _rcItem.top, _rcItem.right, _rcItem.bottom);
    else return CDuiRect(_rcItem.left, _rcItem.top, _rcItem.left - m_iSepWidth, _rcItem.bottom);
}

void CListHeaderItemUI::PaintStatusImage(HDC hDC)
{
    if (IsFocused()) {
        m_uButtonState |= UISTATE_FOCUSED;
    }
    else {
        m_uButtonState &= ~ UISTATE_FOCUSED;
    }

    if ((m_uButtonState & UISTATE_PUSHED) != 0) {
        if (m_sPushedImage.isEmpty() && !m_sNormalImage.isEmpty()) {
            DrawImage(hDC, m_sNormalImage);
        }
        if (!DrawImage(hDC, m_sPushedImage)) {
            m_sPushedImage = String::empty;
        }
    }
    else if ((m_uButtonState & UISTATE_HOT) != 0) {
        if (m_sHotImage.isEmpty() && !m_sNormalImage.isEmpty()) {
            DrawImage(hDC, m_sNormalImage);
        }
        if (!DrawImage(hDC, m_sHotImage)) {
            m_sHotImage = String::empty;
        }
    }
    else if ((m_uButtonState & UISTATE_FOCUSED) != 0) {
        if (m_sFocusedImage.isEmpty() && !m_sNormalImage.isEmpty()) {
            DrawImage(hDC, m_sNormalImage);
        }
        if (!DrawImage(hDC, m_sFocusedImage)) {
            m_sFocusedImage = String::empty;
        }
    }
    else {
        if (!m_sNormalImage.isEmpty()) {
            if (!DrawImage(hDC, m_sNormalImage)) {
                m_sNormalImage = String::empty;
            }
        }
    }

    if (!m_sSepImage.isEmpty()) {
        RECT rcThumb = GetThumbRect();
        rcThumb.left -= _rcItem.left;
        rcThumb.top -= _rcItem.top;
        rcThumb.right -= _rcItem.left;
        rcThumb.bottom -= _rcItem.top;

        m_sSepImageModify = String::empty;
        m_sSepImageModify << "dest='" << rcThumb.left << "," << rcThumb.top << "," << rcThumb.right << "," << rcThumb.bottom << "'";
        if (!DrawImage(hDC, m_sSepImage, m_sSepImageModify)) {
            m_sSepImage = String::empty;
        }
    }
}

void CListHeaderItemUI::PaintText(HDC hDC)
{
    if (m_dwTextColor == 0) {
        m_dwTextColor = _pManager->GetDefaultFontColor();
    }

	RECT rcText = _rcItem;
	rcText.left += m_rcTextPadding.left;
	rcText.top += m_rcTextPadding.top;
	rcText.right -= m_rcTextPadding.right;
	rcText.bottom -= m_rcTextPadding.bottom;

    if (_text.isEmpty()) {
        return;
    }
    int nLinks = 0;
    if (m_bShowHtml) {
        CRenderEngine::DrawHtmlText(hDC, _pManager, rcText, _text, m_dwTextColor, NULL, NULL, nLinks, DT_SINGLELINE | m_uTextStyle);
    }
    else {
        CRenderEngine::DrawText(hDC, _pManager, rcText, _text, m_dwTextColor, m_iFont, DT_SINGLELINE | m_uTextStyle);
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListElementUI::CListElementUI() : 
m_iIndex(-1),
m_pOwner(NULL), 
m_bSelected(false),
m_uButtonState(0)
{
}

LPCTSTR CListElementUI::GetClass() const
{
    return _T("ListElementUI");
}

UINT CListElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN;
}

LPVOID CListElementUI::GetInterface(LPCTSTR pstrName)
{
    if( lstrcmp(pstrName, DUI_CTR_LISTITEM) == 0 ) return static_cast<IListItemUI*>(this);
	if( lstrcmp(pstrName, DUI_CTR_LISTELEMENT) == 0 ) return static_cast<CListElementUI*>(this);
    return CControlUI::GetInterface(pstrName);
}

IListOwnerUI* CListElementUI::GetOwner()
{
    return m_pOwner;
}

void CListElementUI::SetOwner(CControlUI* pOwner)
{
    m_pOwner = static_cast<IListOwnerUI*>(pOwner->GetInterface(_T("IListOwner")));
}

void CListElementUI::SetVisible(bool bVisible)
{
    CControlUI::SetVisible(bVisible);
    if( !IsVisible() && m_bSelected)
    {
        m_bSelected = false;
        if( m_pOwner != NULL ) m_pOwner->SelectItem(-1);
    }
}

void CListElementUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

int CListElementUI::GetIndex() const
{
    return m_iIndex;
}

void CListElementUI::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}

void CListElementUI::Invalidate()
{
    if( !IsVisible() ) return;

    if( GetParent() ) {
        CContainerUI* pParentContainer = static_cast<CContainerUI*>(GetParent()->GetInterface(_T("Container")));
        if( pParentContainer ) {
            RECT rc = pParentContainer->GetPos();
            RECT rcInset = pParentContainer->GetInset();
            rc.left += rcInset.left;
            rc.top += rcInset.top;
            rc.right -= rcInset.right;
            rc.bottom -= rcInset.bottom;
            CScrollBarUI* pVerticalScrollBar = pParentContainer->GetVerticalScrollBar();
            if( pVerticalScrollBar && pVerticalScrollBar->IsVisible() ) rc.right -= pVerticalScrollBar->GetFixedWidth();
            CScrollBarUI* pHorizontalScrollBar = pParentContainer->GetHorizontalScrollBar();
            if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rc.bottom -= pHorizontalScrollBar->GetFixedHeight();

            RECT invalidateRc = _rcItem;
            if( !::IntersectRect(&invalidateRc, &_rcItem, &rc) ) 
            {
                return;
            }

            CControlUI* pParent = GetParent();
            RECT rcTemp;
            RECT rcParent;
            while( pParent = pParent->GetParent() )
            {
                rcTemp = invalidateRc;
                rcParent = pParent->GetPos();
                if( !::IntersectRect(&invalidateRc, &rcTemp, &rcParent) ) 
                {
                    return;
                }
            }

            if( _pManager != NULL ) _pManager->Invalidate(invalidateRc);
        }
        else {
            CControlUI::Invalidate();
        }
    }
    else {
        CControlUI::Invalidate();
    }
}

bool CListElementUI::Activate()
{
    if( !CControlUI::Activate() ) return false;
    if( _pManager != NULL ) _pManager->SendNotify(this, DUI_MSGTYPE_ITEMACTIVATE);
    return true;
}

bool CListElementUI::IsSelected() const
{
    return m_bSelected;
}

bool CListElementUI::Select(bool bSelect)
{
    if( !IsEnabled() ) return false;
    if( bSelect == m_bSelected ) return true;
    m_bSelected = bSelect;
    if( bSelect && m_pOwner != NULL ) m_pOwner->SelectItem(m_iIndex);
    Invalidate();

    return true;
}

bool CListElementUI::IsExpanded() const
{
    return false;
}

bool CListElementUI::Expand(bool /*bExpand = true*/)
{
    return false;
}

void CListElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CControlUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_DBLCLICK )
    {
        if( IsEnabled() ) {
            Activate();
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_KEYDOWN && IsEnabled() )
    {
        if( event.chKey == VK_RETURN ) {
            Activate();
            Invalidate();
            return;
        }
    }
    // An important twist: The list-item will send the event not to its immediate
    // parent but to the "attached" list. A list may actually embed several components
    // in its path to the item, but key-presses etc. needs to go to the actual list.
    if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
}

void CListElementUI::SetAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "selected") {
        Select();
    }
    else {
        CControlUI::SetAttribute(pstrName, pstrValue);
    }
}

void CListElementUI::DrawItemBk(HDC hDC, const RECT& rcItem)
{
    ASSERT(m_pOwner);
    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iBackColor = 0;
    if( !pInfo->bAlternateBk || m_iIndex % 2 == 0 ) iBackColor = pInfo->dwBkColor;
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iBackColor = pInfo->dwHotBkColor;
    }
    if( IsSelected() ) {
        iBackColor = pInfo->dwSelectedBkColor;
    }
    if( !IsEnabled() ) {
        iBackColor = pInfo->dwDisabledBkColor;
    }

    if ( iBackColor != 0 ) {
        CRenderEngine::DrawColor(hDC, _rcItem, GetAdjustColor(iBackColor));
    }

    if (!IsEnabled()) {
        if (!pInfo->sDisabledImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sDisabledImage)) {
                pInfo->sDisabledImage = String::empty;
            }
            else {
                return;
            }
        }
    }
    if (IsSelected()) {
        if (!pInfo->sSelectedImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sSelectedImage)) {
                pInfo->sSelectedImage = String::empty;
            }
            else {
                return;
            }
        }
    }
    if ((m_uButtonState & UISTATE_HOT) != 0) {
        if (!pInfo->sHotImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sHotImage)) {
                pInfo->sHotImage = String::empty;
            }
            else {
                return;
            }
        }
    }

    if (!_bkImageName.isEmpty()) {
        if (!pInfo->bAlternateBk || m_iIndex % 2 == 0) {
            if (!DrawImage(hDC, _bkImageName)) {
                _bkImageName = String::empty;
            }
        }
    }

    if (_bkImageName.isEmpty()) {
        if (!pInfo->sBkImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sBkImage)) {
                pInfo->sBkImage = String::empty;
            }
            else {
                return;
            }
        }
    }

    if (pInfo->dwLineColor != 0) {
        RECT rcLine = { _rcItem.left, _rcItem.bottom - 1, _rcItem.right, _rcItem.bottom - 1 };
        CRenderEngine::DrawLine(hDC, rcLine, 1, GetAdjustColor(pInfo->dwLineColor));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListLabelElementUI::CListLabelElementUI()
{
}

LPCTSTR CListLabelElementUI::GetClass() const
{
    return _T("ListLabelElementUI");
}

LPVOID CListLabelElementUI::GetInterface(LPCTSTR pstrName)
{
    if( lstrcmp(pstrName, DUI_CTR_LISTLABELELEMENT) == 0 ) return static_cast<CListLabelElementUI*>(this);
    return CListElementUI::GetInterface(pstrName);
}

void CListLabelElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CListElementUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN )
    {
        if( IsEnabled() ) {
            _pManager->SendNotify(this, DUI_MSGTYPE_ITEMCLICK);
            Select();
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE ) 
    {
        return;
    }
    if( event.Type == UIEVENT_BUTTONUP )
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( IsEnabled() ) {
            m_uButtonState |= UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( (m_uButtonState & UISTATE_HOT) != 0 ) {
            m_uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    CListElementUI::DoEvent(event);
}

SIZE CListLabelElementUI::EstimateSize(SIZE szAvailable)
{
    if( m_pOwner == NULL ) return CSize(0, 0);

    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    SIZE cXY = _cxyFixed;
    if( cXY.cy == 0 && _pManager != NULL ) {
        cXY.cy = _pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
        cXY.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
    }

    if( cXY.cx == 0 && _pManager != NULL ) {
        RECT rcText = { 0, 0, 9999, cXY.cy };
        if( pInfo->bShowHtml ) {
            int nLinks = 0;
            CRenderEngine::DrawHtmlText(_pManager->GetPaintDC(), _pManager, rcText, _text, 0, NULL, NULL, nLinks, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
        }
        else {
            CRenderEngine::DrawText(_pManager->GetPaintDC(), _pManager, rcText, _text, 0, pInfo->nFont, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
        }
        cXY.cx = rcText.right - rcText.left + pInfo->rcTextPadding.left + pInfo->rcTextPadding.right;        
    }

    return cXY;
}

void CListLabelElementUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if( !::IntersectRect(&_rcPaint, &rcPaint, &_rcItem) ) return;
    DrawItemBk(hDC, _rcItem);
    DrawItemText(hDC, _rcItem);
}

void CListLabelElementUI::DrawItemText(HDC hDC, const RECT& rcItem)
{
    if (_text.isEmpty()) {
        return;
    }

    if (m_pOwner == NULL) {
        return;
    }
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iTextColor = pInfo->dwTextColor;
    if ((m_uButtonState & UISTATE_HOT) != 0) {
        iTextColor = pInfo->dwHotTextColor;
    }
    if (IsSelected()) {
        iTextColor = pInfo->dwSelectedTextColor;
    }
    if (!IsEnabled()) {
        iTextColor = pInfo->dwDisabledTextColor;
    }
    int nLinks = 0;
    RECT rcText = rcItem;
    rcText.left += pInfo->rcTextPadding.left;
    rcText.right -= pInfo->rcTextPadding.right;
    rcText.top += pInfo->rcTextPadding.top;
    rcText.bottom -= pInfo->rcTextPadding.bottom;

    if (pInfo->bShowHtml) {
        CRenderEngine::DrawHtmlText(hDC, _pManager, rcText, _text, iTextColor, NULL, NULL, nLinks, DT_SINGLELINE | pInfo->uTextStyle);
    }
    else {
        CRenderEngine::DrawText(hDC, _pManager, rcText, _text, iTextColor, pInfo->nFont, DT_SINGLELINE | pInfo->uTextStyle);
    }
}


/////////////////////////////////////////////////////////////////////////////////////
//
//

CListTextElementUI::CListTextElementUI() : m_nLinks(0), m_nHoverLink(-1), m_pOwner(NULL)
{
    ::ZeroMemory(&m_rcLinks, sizeof(m_rcLinks));
}

CListTextElementUI::~CListTextElementUI()
{
    String* pText;
    for (int it = 0; it < m_aTexts.GetSize(); ++it) {
        pText = static_cast<String*>(m_aTexts[it]);
        if (pText != NULL) {
            delete pText;
        }
    }

    m_aTexts.Empty();
}

LPCTSTR CListTextElementUI::GetClass() const
{
    return _T("ListTextElementUI");
}

LPVOID CListTextElementUI::GetInterface(LPCTSTR pstrName)
{
    if( lstrcmp(pstrName, DUI_CTR_LISTTEXTELEMENT) == 0 ) return static_cast<CListTextElementUI*>(this);
    return CListLabelElementUI::GetInterface(pstrName);
}

UINT CListTextElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN | ( (IsEnabled() && m_nLinks > 0) ? UIFLAG_SETCURSOR : 0);
}

const String& CListTextElementUI::GetText(int iIndex) const
{
    String* pText = static_cast<String*>(m_aTexts.GetAt(iIndex));
    if (pText != 0) {
        return *pText;
    }
    return String::empty;
}

void CListTextElementUI::SetText(int iIndex, LPCTSTR pstrText)
{
    if (m_pOwner == NULL) {
        return;
    }
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if (iIndex < 0 || iIndex >= pInfo->nColumns) {
        return;
    }
    while (m_aTexts.GetSize() < pInfo->nColumns) {
        m_aTexts.Add(NULL);
    }

    String* pText = static_cast<String*>(m_aTexts[iIndex]);
    if ((pText == NULL && pstrText == NULL) || (pText && *pText == pstrText)) {
        return;
    }

    if (pText != 0) { //by cddjr 2011/10/20
		*pText = pstrText;
    }
    else {
		m_aTexts.SetAt(iIndex, new String(pstrText));
    }
    Invalidate();
}

void CListTextElementUI::SetOwner(CControlUI* pOwner)
{
    CListElementUI::SetOwner(pOwner);
    m_pOwner = static_cast<IListUI*>(pOwner->GetInterface(_T("IList")));
}

String* CListTextElementUI::GetLinkContent(int iIndex)
{
    if (iIndex >= 0 && iIndex < m_nLinks) {
        return &m_sLinks[iIndex];
    }
    return NULL;
}

void CListTextElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CListLabelElementUI::DoEvent(event);
        return;
    }

    // When you hover over a link
    if( event.Type == UIEVENT_SETCURSOR ) {
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
                return;
            }
        }      
    }
    if( event.Type == UIEVENT_BUTTONUP && IsEnabled() ) {
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                _pManager->SendNotify(this, DUI_MSGTYPE_LINK, i);
                return;
            }
        }
    }
    if( m_nLinks > 0 && event.Type == UIEVENT_MOUSEMOVE ) {
        int nHoverLink = -1;
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                nHoverLink = i;
                break;
            }
        }

        if(m_nHoverLink != nHoverLink) {
            Invalidate();
            m_nHoverLink = nHoverLink;
        }
    }
    if( m_nLinks > 0 && event.Type == UIEVENT_MOUSELEAVE ) {
        if(m_nHoverLink != -1) {
            Invalidate();
            m_nHoverLink = -1;
        }
    }
    CListLabelElementUI::DoEvent(event);
}

SIZE CListTextElementUI::EstimateSize(SIZE szAvailable)
{
    TListInfoUI* pInfo = NULL;
    if( m_pOwner ) pInfo = m_pOwner->GetListInfo();

    SIZE cXY = _cxyFixed;
    if( cXY.cy == 0 && _pManager != NULL ) {
        cXY.cy = _pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
        if( pInfo ) cXY.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
    }

    return cXY;
}

void CListTextElementUI::DrawItemText(HDC hDC, const RECT& rcItem)
{
    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iTextColor = pInfo->dwTextColor;

    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iTextColor = pInfo->dwHotTextColor;
    }
    if( IsSelected() ) {
        iTextColor = pInfo->dwSelectedTextColor;
    }
    if( !IsEnabled() ) {
        iTextColor = pInfo->dwDisabledTextColor;
    }
    IListCallbackUI* pCallback = m_pOwner->GetTextCallback();
    //ASSERT(pCallback);
    //if( pCallback == NULL ) return;

    m_nLinks = 0;
    int nLinks = lengthof(m_rcLinks);
    for (int i = 0; i < pInfo->nColumns; ++i) {
        RECT rcItem = { pInfo->rcColumn[i].left, _rcItem.top, pInfo->rcColumn[i].right, _rcItem.bottom };
        rcItem.left += pInfo->rcTextPadding.left;
        rcItem.right -= pInfo->rcTextPadding.right;
        rcItem.top += pInfo->rcTextPadding.top;
        rcItem.bottom -= pInfo->rcTextPadding.bottom;

        String strText;//不使用LPCTSTR，否则限制太多 by cddjr 2011/10/20
        if (pCallback) {
            strText = pCallback->GetItemText(this, m_iIndex, i);
        }
        else {
            strText = GetText(i);
        }
        if (pInfo->bShowHtml) {
            CRenderEngine::DrawHtmlText(hDC, _pManager, rcItem, strText, iTextColor, &m_rcLinks[m_nLinks], &m_sLinks[m_nLinks], nLinks, DT_SINGLELINE | pInfo->uTextStyle);
        }
        else {
            CRenderEngine::DrawText(hDC, _pManager, rcItem, strText, iTextColor, pInfo->nFont, DT_SINGLELINE | pInfo->uTextStyle);
        }

        m_nLinks += nLinks;
        nLinks = lengthof(m_rcLinks) - m_nLinks; 
    }
    for (int i = m_nLinks; i < lengthof(m_rcLinks); ++i) {
        ::ZeroMemory(m_rcLinks + i, sizeof(RECT));
        *((String*)(m_sLinks + i)) = String::empty;
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListContainerElementUI::CListContainerElementUI() : 
m_iIndex(-1),
m_pOwner(NULL), 
m_bSelected(false),
m_uButtonState(0)
{
}

LPCTSTR CListContainerElementUI::GetClass() const
{
    return _T("ListContainerElementUI");
}

UINT CListContainerElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN;
}

LPVOID CListContainerElementUI::GetInterface(LPCTSTR pstrName)
{
    if( lstrcmp(pstrName, DUI_CTR_LISTITEM) == 0 ) return static_cast<IListItemUI*>(this);
	if( lstrcmp(pstrName, DUI_CTR_LISTCONTAINERELEMENT) == 0 ) return static_cast<CListContainerElementUI*>(this);
    return CContainerUI::GetInterface(pstrName);
}

IListOwnerUI* CListContainerElementUI::GetOwner()
{
    return m_pOwner;
}

void CListContainerElementUI::SetOwner(CControlUI* pOwner)
{
    m_pOwner = static_cast<IListOwnerUI*>(pOwner->GetInterface(_T("IListOwner")));
}

void CListContainerElementUI::SetVisible(bool bVisible)
{
    CContainerUI::SetVisible(bVisible);
    if( !IsVisible() && m_bSelected)
    {
        m_bSelected = false;
        if( m_pOwner != NULL ) m_pOwner->SelectItem(-1);
    }
}

void CListContainerElementUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

int CListContainerElementUI::GetIndex() const
{
    return m_iIndex;
}

void CListContainerElementUI::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}

void CListContainerElementUI::Invalidate()
{
    if( !IsVisible() ) return;

    if( GetParent() ) {
        CContainerUI* pParentContainer = static_cast<CContainerUI*>(GetParent()->GetInterface(_T("Container")));
        if( pParentContainer ) {
            RECT rc = pParentContainer->GetPos();
            RECT rcInset = pParentContainer->GetInset();
            rc.left += rcInset.left;
            rc.top += rcInset.top;
            rc.right -= rcInset.right;
            rc.bottom -= rcInset.bottom;
            CScrollBarUI* pVerticalScrollBar = pParentContainer->GetVerticalScrollBar();
            if( pVerticalScrollBar && pVerticalScrollBar->IsVisible() ) rc.right -= pVerticalScrollBar->GetFixedWidth();
            CScrollBarUI* pHorizontalScrollBar = pParentContainer->GetHorizontalScrollBar();
            if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rc.bottom -= pHorizontalScrollBar->GetFixedHeight();

            RECT invalidateRc = _rcItem;
            if( !::IntersectRect(&invalidateRc, &_rcItem, &rc) ) 
            {
                return;
            }

            CControlUI* pParent = GetParent();
            RECT rcTemp;
            RECT rcParent;
            while( pParent = pParent->GetParent() )
            {
                rcTemp = invalidateRc;
                rcParent = pParent->GetPos();
                if( !::IntersectRect(&invalidateRc, &rcTemp, &rcParent) ) 
                {
                    return;
                }
            }

            if( _pManager != NULL ) _pManager->Invalidate(invalidateRc);
        }
        else {
            CContainerUI::Invalidate();
        }
    }
    else {
        CContainerUI::Invalidate();
    }
}

bool CListContainerElementUI::Activate()
{
    if( !CContainerUI::Activate() ) return false;
    if( _pManager != NULL ) _pManager->SendNotify(this, DUI_MSGTYPE_ITEMACTIVATE);
    return true;
}

bool CListContainerElementUI::IsSelected() const
{
    return m_bSelected;
}

bool CListContainerElementUI::Select(bool bSelect)
{
    if( !IsEnabled() ) return false;
    if( bSelect == m_bSelected ) return true;
    m_bSelected = bSelect;
    if( bSelect && m_pOwner != NULL ) m_pOwner->SelectItem(m_iIndex);
    Invalidate();

    return true;
}

bool CListContainerElementUI::IsExpanded() const
{
    return false;
}

bool CListContainerElementUI::Expand(bool /*bExpand = true*/)
{
    return false;
}

void CListContainerElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CContainerUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_DBLCLICK )
    {
        if( IsEnabled() ) {
            Activate();
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_KEYDOWN && IsEnabled() )
    {
        if( event.chKey == VK_RETURN ) {
            Activate();
            Invalidate();
            return;
        }
    }
    if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN )
    {
        if( IsEnabled() ){
            _pManager->SendNotify(this, DUI_MSGTYPE_ITEMCLICK);
            Select();
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_BUTTONUP ) 
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE )
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( IsEnabled() ) {
            m_uButtonState |= UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( (m_uButtonState & UISTATE_HOT) != 0 ) {
            m_uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }

    // An important twist: The list-item will send the event not to its immediate
    // parent but to the "attached" list. A list may actually embed several components
    // in its path to the item, but key-presses etc. needs to go to the actual list.
    if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
}

void CListContainerElementUI::SetAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "selected") {
        Select();
    }
    else {
        CContainerUI::SetAttribute(pstrName, pstrValue);
    }
}

void CListContainerElementUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if (!::IntersectRect(&_rcPaint, &rcPaint, &_rcItem)) {
        return;
    }
    DrawItemBk(hDC, _rcItem);
    CContainerUI::DoPaint(hDC, rcPaint);
}

void CListContainerElementUI::DrawItemText(HDC hDC, const RECT& rcItem)
{
    return;
}

void CListContainerElementUI::DrawItemBk(HDC hDC, const RECT& rcItem)
{
    ASSERT(m_pOwner);
    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iBackColor = 0;
    if( !pInfo->bAlternateBk || m_iIndex % 2 == 0 ) iBackColor = pInfo->dwBkColor;

    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iBackColor = pInfo->dwHotBkColor;
    }
    if( IsSelected() ) {
        iBackColor = pInfo->dwSelectedBkColor;
    }
    if( !IsEnabled() ) {
        iBackColor = pInfo->dwDisabledBkColor;
    }
    if ( iBackColor != 0 ) {
        CRenderEngine::DrawColor(hDC, _rcItem, GetAdjustColor(iBackColor));
    }

    if (!IsEnabled()) {
        if (!pInfo->sDisabledImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sDisabledImage)) {
                pInfo->sDisabledImage = String::empty;
            }
            else {
                return;
            }
        }
    }
    if (IsSelected()) {
        if (!pInfo->sSelectedImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sSelectedImage)) {
                pInfo->sSelectedImage = String::empty;
            }
            else {
                return;
            }
        }
    }
    if ((m_uButtonState & UISTATE_HOT) != 0) {
        if (!pInfo->sHotImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sHotImage)) {
                pInfo->sHotImage = String::empty;
            }
            else {
                return;
            }
        }
    }
    if (!_bkImageName.isEmpty()) {
        if (!pInfo->bAlternateBk || m_iIndex % 2 == 0) {
            if (!DrawImage(hDC, _bkImageName)) {
                _bkImageName = String::empty;
            }
        }
    }

    if (_bkImageName.isEmpty()) {
        if (!pInfo->sBkImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sBkImage)) {
                pInfo->sBkImage = String::empty;
            }
            else {
                return;
            }
        }
    }

    if (pInfo->dwLineColor != 0) {
        RECT rcLine = {_rcItem.left, _rcItem.bottom - 1, _rcItem.right, _rcItem.bottom - 1};
        CRenderEngine::DrawLine(hDC, rcLine, 1, GetAdjustColor(pInfo->dwLineColor));
    }
}

} // namespace zgui

#endif // ZGUI_USE_LIST