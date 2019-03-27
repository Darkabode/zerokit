#include "zgui.h"

#ifdef ZGUI_USE_LIST

namespace zgui {

const String List::CLASS_NAME = ZGUI_LIST;

List::List() :
m_pCallback(0),
m_bScrollSelect(false),
//m_iCurSel(-1),
m_iExpandedItem(-1),
m_bSingleSel(false)
{
    m_pList = new ListBody(this);
    _pHeader = new ListHeader;

    add(_pHeader);
    VerticalLayout::add(m_pList);

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
	m_ListInfo.bShowHLine = true;
	m_ListInfo.bShowVLine = true;
    __stosb((uint8_t*)&m_ListInfo.rcTextPadding, 0, sizeof(m_ListInfo.rcTextPadding));
    __stosb((uint8_t*)&m_ListInfo.rcColumn, 0, sizeof(m_ListInfo.rcColumn));
	__stosb((uint8_t*)&m_ListInfo.szCheckImg, 0, sizeof(m_ListInfo.szCheckImg));
	__stosb((uint8_t*)&m_ListInfo.szIconImg, 0, sizeof(m_ListInfo.szIconImg));
}

const String& List::getClass() const
{
    return CLASS_NAME;
}

UINT List::GetControlFlags() const
{
    return UIFLAG_TABSTOP;
}

LPVOID List::getInterface(const String& name)
{
	if (name == ZGUI_LIST) {
        return static_cast<List*>(this);
    }
    if (name == "IList") {
        return static_cast<IList*>(this);
    }
    if (name == "IListOwner") {
        return static_cast<IListOwner*>(this);
    }
    return VerticalLayout::getInterface(name);
}

Control* List::getItem(int iIndex) const
{
    return m_pList->getItem(iIndex);
}

int List::indexOf(Control* pControl) const
{
    if (pControl->getInterface(ZGUI_LISTHEADER) != NULL) {
        return VerticalLayout::indexOf(pControl);
    }
    // We also need to recognize header sub-items
    if (pControl->getClass().contains(ZGUI_LISTHEADERITEM)) {
        return _pHeader->indexOf(pControl);
    }

    return m_pList->indexOf(pControl);
}

bool List::setItem(int iIndex, Control* pControl)
{
    if (pControl->getInterface(ZGUI_LISTHEADER) != NULL) {
		return VerticalLayout::setItem(iIndex, pControl);
    }
    // We also need to recognize header sub-items
    if (pControl->getClass().contains(ZGUI_LISTHEADERITEM)) {
		return _pHeader->setItem(iIndex, pControl);
    }

    int iOrginIndex = m_pList->indexOf(pControl);
    if (iOrginIndex == -1) {
        return false;
    }
    if (iOrginIndex == iIndex) {
        return true;
    }

    IListItem* pSelectedListItem = NULL;
    //if (m_iCurSel >= 0) {
    //    pSelectedListItem = static_cast<IListItem*>(getItem(m_iCurSel)->getInterface(ZGUI_LISTITEM));
    //}
	if (!m_pList->setItem(iIndex, pControl)) {
        return false;
    }
    int iMinIndex = min(iOrginIndex, iIndex);
    int iMaxIndex = max(iOrginIndex, iIndex);
    for (int i = iMinIndex; i < iMaxIndex + 1; ++i) {
        Control* p = m_pList->getItem(i);
        IListItem* pListItem = static_cast<IListItem*>(p->getInterface(ZGUI_LISTITEM));
        if (pListItem != NULL) {
            pListItem->SetIndex(i);
        }
    }
	UnSelectAllItems();
    //if (m_iCurSel >= 0 && pSelectedListItem != NULL) {
    //    m_iCurSel = pSelectedListItem->GetIndex();
    //}
    return true;
}

int List::getCount() const
{
    return m_pList->getCount();
}

bool List::add(Control* pControl)
{
    // Override the add() method so we can add items specifically to
    // the intended widgets. Headers are assumed to be
    // answer the correct interface so we can add multiple list headers.
    if (pControl->getInterface(ZGUI_LISTHEADER) != 0) {
        if (_pHeader != pControl && _pHeader->getCount() == 0) {
            VerticalLayout::remove(_pHeader);
            _pHeader = static_cast<ListHeader*>(pControl);
        }
        m_ListInfo.nColumns = MIN(_pHeader->getCount(), UILIST_MAX_COLUMNS);
		return VerticalLayout::insert(0, pControl);
    }
    // We also need to recognize header sub-items
    if (pControl->getClass().contains(ZGUI_LISTHEADERITEM)) {
        bool ret = _pHeader->add(pControl);
        m_ListInfo.nColumns = MIN(_pHeader->getCount(), UILIST_MAX_COLUMNS);
        return ret;
    }
    // The list items should know about us
    IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface(ZGUI_LISTITEM));
    if (pListItem != 0) {
        pListItem->SetOwner(this);
        pListItem->SetIndex(getCount());
    }
    return m_pList->add(pControl);
}

bool List::insert(int iIndex, Control* pControl)
{
    // Override the insert() method so we can add items specifically to
    // the intended widgets. Headers and are assumed to be
    // answer the correct interface so we can add multiple list headers.
    if (pControl->getInterface(ZGUI_LISTHEADER) != 0) {
        if (_pHeader != pControl && _pHeader->getCount() == 0) {
            VerticalLayout::remove(_pHeader);
            _pHeader = static_cast<ListHeader*>(pControl);
        }
        m_ListInfo.nColumns = MIN(_pHeader->getCount(), UILIST_MAX_COLUMNS);
		return VerticalLayout::insert(0, pControl);
    }
    // We also need to recognize header sub-items
    if (pControl->getClass().contains(ZGUI_LISTHEADERITEM)) {
		bool ret = _pHeader->insert(iIndex, pControl);
        m_ListInfo.nColumns = MIN(_pHeader->getCount(), UILIST_MAX_COLUMNS);
        return ret;
    }
	if (!m_pList->insert(iIndex, pControl)) {
		return false;
	}

    // The list items should know about us
    IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface(ZGUI_LISTITEM));
    if (pListItem != NULL) {
        pListItem->SetOwner(this);
        pListItem->SetIndex(iIndex);
    }

    for(int i = iIndex + 1; i < m_pList->getCount(); ++i) {
        Control* p = m_pList->getItem(i);
        pListItem = static_cast<IListItem*>(p->getInterface(ZGUI_LISTITEM));
        if (pListItem != NULL) {
            pListItem->SetIndex(i);
        }
    }
	//if (m_iCurSel >= iIndex) {
	//	m_iCurSel += 1;
	//}
	UnSelectAllItems();
    return true;
}

bool List::remove(Control* pControl)
{
	if (pControl->getInterface(ZGUI_LISTHEADER) != 0) {
		return VerticalLayout::remove(pControl);
	}
    // We also need to recognize header sub-items
	if (pControl->getClass().contains(ZGUI_LISTHEADERITEM)) {
		return _pHeader->remove(pControl);
	}

    int iIndex = m_pList->indexOf(pControl);
	if (iIndex == -1) {
		return false;
	}

	if (!m_pList->removeAt(iIndex)) {
		return false;
	}

    for (int i = iIndex; i < m_pList->getCount(); ++i) {
        Control* p = m_pList->getItem(i);
        IListItem* pListItem = static_cast<IListItem*>(p->getInterface(ZGUI_LISTITEM));
        if (pListItem != 0) {
            pListItem->SetIndex(i);
        }
    }

	m_aSelItems.remove(m_aSelItems.indexOf(iIndex));

 //   if (iIndex == m_iCurSel && m_iCurSel >= 0) {
 //       int iSel = m_iCurSel;
 //       m_iCurSel = -1;
 //       SelectItem(FindSelectable(iSel, false));
 //   }
	//else if (iIndex < m_iCurSel) {
	//	m_iCurSel -= 1;
	//}
    return true;
}

bool List::removeAt(int iIndex)
{
	if (!m_pList->removeAt(iIndex)) {
		return false;
	}

    for (int i = iIndex; i < m_pList->getCount(); ++i) {
        Control* p = m_pList->getItem(i);
        IListItem* pListItem = static_cast<IListItem*>(p->getInterface("ListItem"));
		if (pListItem != 0) {
			pListItem->SetIndex(i);
		}
    }

	m_aSelItems.remove(m_aSelItems.indexOf(iIndex));

    //if (iIndex == m_iCurSel && m_iCurSel >= 0) {
    //    int iSel = m_iCurSel;
    //    m_iCurSel = -1;
    //    SelectItem(FindSelectable(iSel, false));
    //}
	//else if( iIndex < m_iCurSel ) {
	//	m_iCurSel -= 1;
	//}
    return true;
}

void List::removeAll()
{
    //m_iCurSel = -1;
    m_iExpandedItem = -1;
    m_pList->removeAll();
}

void List::SetPos(RECT rc)
{
    VerticalLayout::SetPos(rc);
	if (_pHeader == NULL) {
		return;
	}
    // Determine general list information and the size of header columns
    m_ListInfo.nColumns = MIN(_pHeader->getCount(), UILIST_MAX_COLUMNS);
    // The header/columns may or may not be visible at runtime. In either case
    // we should determine the correct dimensions...

    if (!_pHeader->IsVisible()) {
        for (int it = 0; it < _pHeader->getCount(); ++it) {
            static_cast<Control*>(_pHeader->getItem(it))->SetInternVisible(true);
        }
        _pHeader->SetPos(Rect(rc.left, 0, rc.right, 0));
    }
    int iOffset = m_pList->getScrollPos().cx;
    for (int i = 0; i < m_ListInfo.nColumns; ++i) {
        Control* pControl = static_cast<Control*>(_pHeader->getItem(i));
		if (!pControl->IsVisible() || pControl->IsFloat()) {
			continue;
		}

        RECT rcPos = pControl->GetPos();
        if (iOffset > 0) {
            rcPos.left -= iOffset;
            rcPos.right -= iOffset;
            pControl->SetPos(rcPos);
        }
        m_ListInfo.rcColumn[i] = pControl->GetPos();
    }
    if (!_pHeader->IsVisible()) {
        for (int it = 0; it < _pHeader->getCount(); ++it) {
            static_cast<Control*>(_pHeader->getItem(it))->SetInternVisible(false);
        }
    }
}

int List::GetMinSelItemIndex()
{
	if (m_aSelItems.size() <= 0) {
		return -1;
	}
	int min = (int)m_aSelItems[0];
	int index;
	for (int i = 1; i < m_aSelItems.size(); ++i) {
		index = (int)m_aSelItems[i];
		if (min > index) {
			min = index;
		}
	}
	return min;
}

int List::GetMaxSelItemIndex()
{
	if (m_aSelItems.size() <= 0) {
		return -1;
	}
	int max = (int)m_aSelItems[0];
	int index;
	for (int i = 1; i < m_aSelItems.size(); ++i) {
		index = (int)m_aSelItems[i];
		if (max < index) {
			max = index;
		}
	}
	return max;
}

void List::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( _pParent != NULL ) _pParent->DoEvent(event);
        else VerticalLayout::DoEvent(event);
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
				{
					if (m_aSelItems.size() > 0) {
						int index = GetMinSelItemIndex() - 1;
						UnSelectAllItems();
						index > 0 ? SelectItem(index, true) : SelectItem(0, true);
					}
				}
				return;
			case VK_DOWN:
				{
					if (m_aSelItems.size() > 0) {
						int index = GetMaxSelItemIndex() + 1;
						UnSelectAllItems();
						index + 1 > m_pList->getCount() ? SelectItem(getCount() - 1, true) : SelectItem(index, true);
					}
				}
				return;
			case VK_PRIOR:
				pageUp();
				return;
			case VK_NEXT:
				pageDown();
				return;
			case VK_HOME:
				{
					if (m_pList->getCount() > 0) {
						SelectItem(0, true);
					}
				}
				return;
			case VK_END:
				{
					if (m_pList->getCount() > 0) {
						SelectItem(m_pList->getCount() - 1, true);
					}
				}
				return;
			case 0x41:
				{
					if (!m_bSingleSel && (fn_GetKeyState(VK_CONTROL) & 0x8000)) {
						SelectAllItems();
					}
				}
				return;
	//        case VK_RETURN:
	//            if( m_iCurSel != -1 ) getItem(m_iCurSel)->Activate();
	//            return;
				}
			break;
		case UIEVENT_SCROLLWHEEL:
			{
				switch (LOWORD(event.wParam)) {
					case SB_LINEUP:
						if (m_bScrollSelect && m_bSingleSel) {
							SelectItem(FindSelectable(m_aSelItems[0] - 1, false), true);
						}
						else {
							lineUp();
						}
						return;
					case SB_LINEDOWN:
						if (m_bScrollSelect && m_bSingleSel) {
							SelectItem(FindSelectable(m_aSelItems[0] + 1, true), true);
						}
						else {
							lineDown();
						}
						return;
				}
			}
			break;
    }
    VerticalLayout::DoEvent(event);
}

ListHeader* List::GetHeader() const
{
    return _pHeader;
}

Container* List::GetList() const
{
    return m_pList;
}

bool List::GetScrollSelect()
{
    return m_bScrollSelect;
}

void List::SetScrollSelect(bool bScrollSelect)
{
    m_bScrollSelect = bScrollSelect;
}

int List::GetCurSel() const
{
	if (m_aSelItems.size() <= 0) {
		return -1;
	}
	else {
		return m_aSelItems[0];
	}

	return -1;
}

bool List::SelectItem(int iIndex, bool bTakeFocus)
{
	if (iIndex < 0) {
		return true;
	}

    Control* pControl = getItem(iIndex);
	if (pControl == 0 || !pControl->IsVisible() || !pControl->IsEnabled()) {
		return false;
	}

    IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
	if (pListItem == 0) {
		return false;
	}
	UnSelectAllItems();
	
	if (m_bSingleSel && m_aSelItems.size() > 0) {
		Control* pControl = getItem(m_aSelItems[0]);
		if (pControl != 0) {
			IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
			if (pListItem != 0) {
				pListItem->Select(false);
			}
		}
	}

    if (!pListItem->Select(true)) {
        return false;
    }

	m_aSelItems.add(iIndex);

	EnsureVisible(iIndex);
	if (bTakeFocus) {
		pControl->SetFocus();
	}
    if (_pManager != 0) {
		_pManager->SendNotify(this, ZGUI_MSGTYPE_ITEMSELECT, iIndex);
    }

    return true;
}

ListInfo* List::GetListInfo()
{
    return &m_ListInfo;
}

int List::GetChildPadding() const
{
    return m_pList->getChildPadding();
}

void List::SetChildPadding(int iPadding)
{
    m_pList->setChildPadding(iPadding);
}

void List::SetItemFont(int index)
{
    m_ListInfo.nFont = index;
    NeedUpdate();
}

void List::SetItemTextStyle(UINT uStyle)
{
    m_ListInfo.uTextStyle = uStyle;
    NeedUpdate();
}

void List::SetItemTextPadding(RECT rc)
{
    m_ListInfo.rcTextPadding = rc;
    NeedUpdate();
}

RECT List::GetItemTextPadding() const
{
	return m_ListInfo.rcTextPadding;
}

void List::SetItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwTextColor = dwTextColor;
    Invalidate();
}

void List::SetItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwBkColor = dwBkColor;
    Invalidate();
}

void List::SetItemBkImage(const String& pStrImage)
{
    m_ListInfo.sBkImage = pStrImage;
    Invalidate();
}

void List::SetAlternateBk(bool bAlternateBk)
{
    m_ListInfo.bAlternateBk = bAlternateBk;
    Invalidate();
}

DWORD List::GetItemTextColor() const
{
	return m_ListInfo.dwTextColor;
}

DWORD List::GetItemBkColor() const
{
	return m_ListInfo.dwBkColor;
}

const String& List::GetItemBkImage() const
{
	return m_ListInfo.sBkImage;
}

bool List::IsAlternateBk() const
{
    return m_ListInfo.bAlternateBk;
}

void List::SetSelectedItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwSelectedTextColor = dwTextColor;
    Invalidate();
}

void List::SetSelectedItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwSelectedBkColor = dwBkColor;
    Invalidate();
}

void List::SetSelectedItemImage(const String& pStrImage)
{
    m_ListInfo.sSelectedImage = pStrImage;
    Invalidate();
}

DWORD List::GetSelectedItemTextColor() const
{
	return m_ListInfo.dwSelectedTextColor;
}

DWORD List::GetSelectedItemBkColor() const
{
	return m_ListInfo.dwSelectedBkColor;
}

const String& List::GetSelectedItemImage() const
{
	return m_ListInfo.sSelectedImage;
}

void List::SetHotItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwHotTextColor = dwTextColor;
    Invalidate();
}

void List::SetHotItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwHotBkColor = dwBkColor;
    Invalidate();
}

void List::SetHotItemImage(const String& pStrImage)
{
    m_ListInfo.sHotImage = pStrImage;
    Invalidate();
}

DWORD List::GetHotItemTextColor() const
{
	return m_ListInfo.dwHotTextColor;
}
DWORD List::GetHotItemBkColor() const
{
	return m_ListInfo.dwHotBkColor;
}

const String& List::GetHotItemImage() const
{
	return m_ListInfo.sHotImage;
}

void List::SetDisabledItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwDisabledTextColor = dwTextColor;
    Invalidate();
}

void List::SetDisabledItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwDisabledBkColor = dwBkColor;
    Invalidate();
}

void List::SetDisabledItemImage(const String& pStrImage)
{
    m_ListInfo.sDisabledImage = pStrImage;
    Invalidate();
}

DWORD List::GetDisabledItemTextColor() const
{
	return m_ListInfo.dwDisabledTextColor;
}

DWORD List::GetDisabledItemBkColor() const
{
	return m_ListInfo.dwDisabledBkColor;
}

const String& List::GetDisabledItemImage() const
{
	return m_ListInfo.sDisabledImage;
}

DWORD List::GetItemLineColor() const
{
	return m_ListInfo.dwLineColor;
}

void List::SetItemLineColor(DWORD dwLineColor)
{
    m_ListInfo.dwLineColor = dwLineColor;
    Invalidate();
}

bool List::IsItemShowHtml()
{
    return m_ListInfo.bShowHtml;
}

void List::SetItemShowHtml(bool bShowHtml)
{
    if( m_ListInfo.bShowHtml == bShowHtml ) return;

    m_ListInfo.bShowHtml = bShowHtml;
    NeedUpdate();
}

void List::SetMultiExpanding(bool bMultiExpandable)
{
    m_ListInfo.bMultiExpandable = bMultiExpandable;
}

bool List::ExpandItem(int iIndex, bool bExpand /*= true*/)
{
    if (m_iExpandedItem >= 0 && !m_ListInfo.bMultiExpandable) {
        Control* pControl = getItem(m_iExpandedItem);
        if (pControl != 0) {
            IListItem* pItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
			if (pItem != 0) {
				pItem->Expand(false);
			}
        }
        m_iExpandedItem = -1;
    }
    if (bExpand) {
        Control* pControl = getItem(iIndex);
		if (pControl == 0) {
			return false;
		}
		if (!pControl->IsVisible()) {
			return false;
		}
        IListItem* pItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
		if (pItem == 0) {
			return false;
		}
        m_iExpandedItem = iIndex;
        if (!pItem->Expand(true)) {
            m_iExpandedItem = -1;
            return false;
        }
    }
    NeedUpdate();
    return true;
}

int List::GetExpandedItem() const
{
    return m_iExpandedItem;
}

void List::EnsureVisible(int iIndex)
{
    RECT rcItem = m_pList->getItem(iIndex)->GetPos();
    RECT rcList = m_pList->GetPos();
    RECT rcListInset = m_pList->getInset();

    rcList.left += rcListInset.left;
    rcList.top += rcListInset.top;
    rcList.right -= rcListInset.right;
    rcList.bottom -= rcListInset.bottom;

    ScrollBar* pHorizontalScrollBar = m_pList->getHorizontalScrollBar();
	if (pHorizontalScrollBar && pHorizontalScrollBar->IsVisible()) {
		rcList.bottom -= pHorizontalScrollBar->GetFixedHeight();
	}

    int iPos = m_pList->getScrollPos().cy;
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

void List::Scroll(int dx, int dy)
{
	if (dx == 0 && dy == 0) {
		return;
	}
    SIZE sz = m_pList->getScrollPos();
    m_pList->setScrollPos(Size(sz.cx + dx, sz.cy + dy));
}

void List::setAttribute(const String& pstrName, const String& pstrValue)
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
	else if (pstrName == "multipleitem") {
		SetMultipleItem(pstrValue == "true");
	}
	else if (pstrName == "showvline") {
		SetShowVLine(pstrValue == "true");
	}
	else if (pstrName == "showhline") {
		SetShowHLine(pstrValue == "true");
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
        RECT rcTextPadding;
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcTextPadding.left, (int&)rcTextPadding.top, (int&)rcTextPadding.right, (int&)rcTextPadding.bottom)) {
            SetItemTextPadding(rcTextPadding);
        }
	}
	else if (pstrName == "itemcheckimgsize") {
		SIZE szCheckImg;
		if (Helper::splitString(pstrValue, ",", String::empty, (int&)szCheckImg.cx, (int&)szCheckImg.cy)) {
			SetCheckImgSize(szCheckImg);
		}
	}
	else if (pstrName == "itemiconimgsize") {
		SIZE szIconImg;
		if (Helper::splitString(pstrValue, ",", String::empty, (int&)szIconImg.cx, (int&)szIconImg.cy)) {
			SetIconImgSize(szIconImg);
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
        VerticalLayout::setAttribute(pstrName, pstrValue);
    }
}

IListCallback* List::GetTextCallback() const
{
    return m_pCallback;
}

void List::SetTextCallback(IListCallback* pCallback)
{
    m_pCallback = pCallback;
}

SIZE List::getScrollPos() const
{
    return m_pList->getScrollPos();
}

SIZE List::getScrollRange() const
{
    return m_pList->getScrollRange();
}

void List::setScrollPos(SIZE szPos)
{
    m_pList->setScrollPos(szPos);
}

void List::lineUp()
{
    m_pList->lineUp();
}

void List::lineDown()
{
    m_pList->lineDown();
}

void List::pageUp()
{
    m_pList->pageUp();
}

void List::pageDown()
{
    m_pList->pageDown();
}

void List::homeUp()
{
    m_pList->homeUp();
}

void List::endDown()
{
    m_pList->endDown();
}

void List::lineLeft()
{
    m_pList->lineLeft();
}

void List::lineRight()
{
    m_pList->lineRight();
}

void List::pageLeft()
{
    m_pList->pageLeft();
}

void List::pageRight()
{
    m_pList->pageRight();
}

void List::homeLeft()
{
    m_pList->homeLeft();
}

void List::endRight()
{
    m_pList->endRight();
}

void List::enableScrollBar(bool bEnableVertical, bool bEnableHorizontal)
{
    m_pList->enableScrollBar(bEnableVertical, bEnableHorizontal);
}

ScrollBar* List::getVerticalScrollBar() const
{
    return m_pList->getVerticalScrollBar();
}

ScrollBar* List::getHorizontalScrollBar() const
{
    return m_pList->getHorizontalScrollBar();
}

bool List::SelectMultiItem(int iIndex, bool bTakeFocus /*= false*/)
{
	if (m_bSingleSel) {
		return SelectItem(iIndex, bTakeFocus);
	}

	if (iIndex < 0) {
		return false;
	}
	Control* pControl = getItem(iIndex);
	if (pControl == 0 || !pControl->IsVisible() || !pControl->IsEnabled()) {
		return false;
	}
	IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
	if (pListItem == 0) {
		return false;
	}

	if (m_aSelItems.indexOf(iIndex) >= 0) {
		return false;
	}

	if (m_bSingleSel && m_aSelItems.size() > 0) {
		Control* pControl = getItem(m_aSelItems[0]);
		if (pControl != 0) {
			IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
			if (pListItem != 0) {
				pListItem->Select(false);
			}

		}
	}

	if (!pListItem->Select(true)) {
		return false;
	}

	m_aSelItems.add(iIndex);

	EnsureVisible(iIndex);
	if (bTakeFocus) {
		pControl->SetFocus();
	}
	if (_pManager != 0) {
		_pManager->SendNotify(this, "itemselect", iIndex);
	}

	return true;
}

void List::SetSingleSelect(bool bSingleSel)
{
	m_bSingleSel = bSingleSel;
	UnSelectAllItems();
}

bool List::GetSingleSelect() const
{
	return m_bSingleSel;
}

bool List::UnSelectItem(int iIndex)
{
	if (iIndex < 0) {
		return false;
	}
	Control* pControl = getItem(iIndex);
	if (pControl == 0 || !pControl->IsVisible() || !pControl->IsEnabled()) {
		return false;
	}
	IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
	if (pListItem == 0) {
		return false;
	}

	int aIndex = m_aSelItems.indexOf(iIndex);
	if (aIndex < 0) {
		return false;
	}

	if (!pListItem->Select(false)) {
		return false;
	}

	m_aSelItems.remove(aIndex);

	return true;
}

void List::SelectAllItems()
{
	UnSelectAllItems();
	Control* pControl;
	for (int i = 0; i < getCount(); ++i) {
		pControl = getItem(i);
		if (pControl == 0 || !pControl->IsVisible() || !pControl->IsEnabled()) {
			continue;
		}

		IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
		if (pListItem == 0 || !pListItem->Select(true)) {
			continue;
		}
		m_aSelItems.add(i);
	}
}

void List::UnSelectAllItems()
{
	Control* pControl;
	int itemIndex;
	for (int i = 0; i < m_aSelItems.size(); ++i) {
		itemIndex = m_aSelItems[i];
		pControl = getItem(itemIndex);
		if (pControl == 0 || !pControl->IsVisible() || !pControl->IsEnabled()) {
			continue;
		}
		IListItem* pListItem = static_cast<IListItem*>(pControl->getInterface("ListItem"));
		if (pListItem == 0 || !pListItem->Select(false)) {
			continue;
		}
	}
	m_aSelItems.clear();
}

int List::GetSelectItemCount() const
{
	return m_aSelItems.size();
}

int List::GetNextSelItem(int nItem) const
{
	if (m_aSelItems.size() <= 0) {
		return -1;
	}

	if (nItem < 0) {
		return m_aSelItems[0];
	}
	int aIndex = m_aSelItems.indexOf(nItem);
	if (aIndex < 0) {
		return -1;
	}
	if (aIndex + 1 > m_aSelItems.size() - 1) {
		return -1;
	}
	return m_aSelItems[aIndex + 1];
}

void List::SetCheckImgSize(SIZE szCheckImg)
{
	m_ListInfo.szCheckImg = szCheckImg;
}

void List::SetIconImgSize(SIZE szIconImg)
{
	m_ListInfo.szIconImg = szIconImg;
}

void List::SetShowVLine(bool bVLine)
{
	m_ListInfo.bShowVLine = bVLine;
}

void List::SetShowHLine(bool bHLine)
{
	m_ListInfo.bShowHLine = bHLine;
}

SIZE List::GetCheckImgSize() const
{
	return m_ListInfo.szCheckImg;
}

SIZE List::GetIconImgSize() const
{
	return m_ListInfo.szIconImg;
}

bool List::IsShowVLine() const
{
	return m_ListInfo.bShowVLine;
}

bool List::IsShowHLine() const
{
	return m_ListInfo.bShowHLine;
}

BOOL List::SortItems(PULVCompareFunc pfnCompare, UINT_PTR dwData)
{
	if (!m_pList) {
		return FALSE;
	}
	return m_pList->SortItems(pfnCompare, dwData);
}

void List::SetMultipleItem(bool bMultipleable)
{
	m_bSingleSel = !bMultipleable;
}
//************************************
// Method:    GetMultipleItem
// FullName:  CListUI::GetMultipleItem
// Access:    public 
// Returns:   bool
// Qualifier: const
// Note:	  
//************************************
bool List::GetMultipleItem() const
{
	return !m_bSingleSel;
}

/////////////////////////////////////////////////////////////////////////////////////

ListBody::ListBody(List* pOwner) :
_pOwner(pOwner)
{
    zgui_assert(_pOwner);
}

void ListBody::setScrollPos(SIZE szPos)
{
    int cx = 0;
    int cy = 0;
    if (_pVerticalScrollBar && _pVerticalScrollBar->IsVisible()) {
        int iLastScrollPos = _pVerticalScrollBar->GetScrollPos();
        _pVerticalScrollBar->SetScrollPos(szPos.cy);
        cy = _pVerticalScrollBar->GetScrollPos() - iLastScrollPos;
    }

    if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
        int iLastScrollPos = _pHorizontalScrollBar->GetScrollPos();
        _pHorizontalScrollBar->SetScrollPos(szPos.cx);
        cx = _pHorizontalScrollBar->GetScrollPos() - iLastScrollPos;
    }

	if (cx == 0 && cy == 0) {
		return;
	}

    RECT rcPos;
    for (int it2 = 0; it2 < _items.size(); ++it2) {
        Control* pControl = _items.getUnchecked(it2);
		if (!pControl->IsVisible() || pControl->IsFloat()) {
			continue;
		}

        rcPos = pControl->GetPos();
        rcPos.left -= cx;
        rcPos.right -= cx;
        rcPos.top -= cy;
        rcPos.bottom -= cy;
        pControl->SetPos(rcPos);
    }

    Invalidate();

    if (cx != 0 && _pOwner) {
        ListHeader* pHeader = _pOwner->GetHeader();
		if (pHeader == 0) {
			return;
		}
        ListInfo* pInfo = _pOwner->GetListInfo();
        pInfo->nColumns = MIN(pHeader->getCount(), UILIST_MAX_COLUMNS);

        if( !pHeader->IsVisible() ) {
            for (int it = 0; it < pHeader->getCount(); ++it) {
                pHeader->getItem(it)->SetInternVisible(true);
            }
        }
        for (int i = 0; i < pInfo->nColumns; ++i) {
            Control* pControl = pHeader->getItem(i);
			if (!pControl->IsVisible() || pControl->IsFloat()) {
				continue;
			}

            RECT rcPos = pControl->GetPos();
            rcPos.left -= cx;
            rcPos.right -= cx;
            pControl->SetPos(rcPos);
            pInfo->rcColumn[i] = pControl->GetPos();
        }
        if (!pHeader->IsVisible()) {
            for (int it = 0; it < pHeader->getCount(); ++it) {
                pHeader->getItem(it)->SetInternVisible(false);
            }
        }
    }
}

void ListBody::SetPos(RECT rc)
{
    Control::SetPos(rc);
    rc = _rcItem;

    // Adjust for inset
    rc.left += _rcInset.left;
    rc.top += _rcInset.top;
    rc.right -= _rcInset.right;
    rc.bottom -= _rcInset.bottom;
	if (_pVerticalScrollBar != 0 && _pVerticalScrollBar->IsVisible()) {
		rc.right -= _pVerticalScrollBar->GetFixedWidth();
	}
	if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
		rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();
	}

    // Determine the minimum size
    SIZE szAvailable = { rc.right - rc.left, rc.bottom - rc.top };
	if (_pHorizontalScrollBar != 0 && _pHorizontalScrollBar->IsVisible()) {
		szAvailable.cx += _pHorizontalScrollBar->GetScrollRange();
	}

    int cxNeeded = 0;
    int nAdjustables = 0;
    int cyFixed = 0;
    int nEstimateNum = 0;
    for (int it1 = 0; it1 < _items.size(); ++it1) {
        Control* pControl = _items.getUnchecked(it1);
		if (!pControl->IsVisible() || pControl->IsFloat()) {
			continue;
		}

        SIZE sz = pControl->EstimateSize(szAvailable);
        if (sz.cy == 0) {
            ++nAdjustables;
        }
        else {
			if (sz.cy < pControl->GetMinHeight()) {
				sz.cy = pControl->GetMinHeight();
			}
			if (sz.cy > pControl->GetMaxHeight()) {
				sz.cy = pControl->GetMaxHeight();
			}
        }
        cyFixed += sz.cy + pControl->GetPadding().top + pControl->GetPadding().bottom;

        RECT rcPadding = pControl->GetPadding();
        sz.cx = MAX(sz.cx, 0);
		if (sz.cx < pControl->GetMinWidth()) {
			sz.cx = pControl->GetMinWidth();
		}
		if (sz.cx > pControl->GetMaxWidth()) {
			sz.cx = pControl->GetMaxWidth();
		}
        cxNeeded = MAX(cxNeeded, sz.cx);
        ++nEstimateNum;
    }
    cyFixed += (nEstimateNum - 1) * m_iChildPadding;

    if (_pOwner != 0) {
        ListHeader* pHeader = _pOwner->GetHeader();
        if (pHeader != 0 && pHeader->getCount() > 0) {
            cxNeeded = MAX(0, pHeader->EstimateSize(Size(rc.right - rc.left, rc.bottom - rc.top)).cx);
        }
    }

    // Place elements
    int cyNeeded = 0;
    int cyExpand = 0;
    if( nAdjustables > 0 ) cyExpand = MAX(0, (szAvailable.cy - cyFixed) / nAdjustables);
    // Position the elements
    SIZE szRemaining = szAvailable;
    int iPosY = rc.top;
    if (_pVerticalScrollBar && _pVerticalScrollBar->IsVisible()) {
        iPosY -= _pVerticalScrollBar->GetScrollPos();
    }
    int iPosX = rc.left;
    if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
        iPosX -= _pHorizontalScrollBar->GetScrollPos();
    }
    int iAdjustable = 0;
    int cyFixedRemaining = cyFixed;
    for (int it2 = 0; it2 < _items.size(); ++it2) {
        Control* pControl = _items.getUnchecked(it2);
		if (!pControl->IsVisible()) {
			continue;
		}
        if( pControl->IsFloat() ) {
            SetFloatPos(it2);
            continue;
        }

        RECT rcPadding = pControl->GetPadding();
        szRemaining.cy -= rcPadding.top;
        SIZE sz = pControl->EstimateSize(szRemaining);
        if (sz.cy == 0) {
            ++iAdjustable;
            sz.cy = cyExpand;
            // Distribute remaining to last element (usually round-off left-overs)
            if (iAdjustable == nAdjustables) {
                sz.cy = MAX(0, szRemaining.cy - rcPadding.bottom - cyFixedRemaining);
            } 
			if (sz.cy < pControl->GetMinHeight()) {
				sz.cy = pControl->GetMinHeight();
			}
			if (sz.cy > pControl->GetMaxHeight()) {
				sz.cy = pControl->GetMaxHeight();
			}
        }
        else {
			if (sz.cy < pControl->GetMinHeight()) {
				sz.cy = pControl->GetMinHeight();
			}
			if (sz.cy > pControl->GetMaxHeight()) {
				sz.cy = pControl->GetMaxHeight();
			}
            cyFixedRemaining -= sz.cy;
        }

        sz.cx = MAX(cxNeeded, szAvailable.cx - rcPadding.left - rcPadding.right);

		if (sz.cx < pControl->GetMinWidth()) {
			sz.cx = pControl->GetMinWidth();
		}
		if (sz.cx > pControl->GetMaxWidth()) {
			sz.cx = pControl->GetMaxWidth();
		}

        RECT rcCtrl = { iPosX + rcPadding.left, iPosY + rcPadding.top, iPosX + rcPadding.left + sz.cx, iPosY + sz.cy + rcPadding.top + rcPadding.bottom };
        pControl->SetPos(rcCtrl);

        iPosY += sz.cy + m_iChildPadding + rcPadding.top + rcPadding.bottom;
        cyNeeded += sz.cy + rcPadding.top + rcPadding.bottom;
        szRemaining.cy -= sz.cy + m_iChildPadding + rcPadding.bottom;
    }
    cyNeeded += (nEstimateNum - 1) * m_iChildPadding;

    if (_pHorizontalScrollBar != 0) {
        if (cxNeeded > rc.right - rc.left) {
            if (_pHorizontalScrollBar->IsVisible()) {
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
            if (_pHorizontalScrollBar->IsVisible()) {
                _pHorizontalScrollBar->SetVisible(false);
                _pHorizontalScrollBar->SetScrollRange(0);
                _pHorizontalScrollBar->SetScrollPos(0);
                rc.bottom += _pHorizontalScrollBar->GetFixedHeight();
            }
        }
    }

    // Process the scrollbar
    processScrollBar(rc, cxNeeded, cyNeeded);
}

void ListBody::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
		if (_pOwner != 0) {
			_pOwner->DoEvent(event);
		}
		else {
			Control::DoEvent(event);
		}
        return;
    }

	if (_pOwner != 0) {
		_pOwner->DoEvent(event);
	}
	else {
		Control::DoEvent(event);
	}
}

int ListBody::ItemComareFunc(const void *item1, const void *item2)
{
	Control* pControl1 = *(Control**)item1;
	Control* pControl2 = *(Control**)item2;
	return m_pCompareFunc((UINT_PTR)pControl1, (UINT_PTR)pControl2, m_compareData);
}

int ListBody::ItemComareFunc(void *pvlocale, const void *item1, const void *item2)
{
	ListBody* pThis = (ListBody*)pvlocale;
	if (!pThis || !item1 || !item2) {
		return 0;
	}
	return pThis->ItemComareFunc(item1, item2);
}

BOOL ListBody::SortItems(PULVCompareFunc pfnCompare, UINT_PTR dwData)
{
	if (!pfnCompare) {
		return FALSE;
	}
	m_pCompareFunc = pfnCompare;
	Control** pData = _items.getRawDataPointer();
	QuickSort::sort(_items.getRawDataPointer(), _items.size(), sizeof(Control*), ListBody::ItemComareFunc, this);
	IListItem* pItem = 0;
	for (int i = 0; i < _items.size(); ++i) {
		pItem = (IListItem*)(_items.getUnchecked(i)->getInterface("ListItem"));
		if (pItem) {
			pItem->SetIndex(i);
			pItem->Select(false);
		}
	}
	_pOwner->SelectItem(-1);
	if (_pManager) {
		SetPos(GetPos());
		Invalidate();
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////

const String ListHeader::CLASS_NAME = ZGUI_LISTHEADER;

ListHeader::ListHeader()
{
}

const String& ListHeader::getClass() const
{
    return CLASS_NAME;
}

LPVOID ListHeader::getInterface(const String& name)
{
	if (name == ZGUI_LISTHEADER) {
		return this;
	}
    return HorizontalLayout::getInterface(name);
}

SIZE ListHeader::EstimateSize(SIZE szAvailable)
{
    SIZE cXY = {0, _cxyFixed.cy};
	if (cXY.cy == 0 && _pManager != 0) {
		for (int it = 0; it < _items.size(); ++it) {
			cXY.cy = MAX(cXY.cy, _items.getUnchecked(it)->EstimateSize(szAvailable).cy);
		}
		int nMin = _pManager->GetDefaultFontInfo()->tm.tmHeight + 6;
		cXY.cy = MAX(cXY.cy,nMin);
	}

    for (int it = 0; it < _items.size(); ++it) {
        cXY.cx += _items.getUnchecked(it)->EstimateSize(szAvailable).cx;
    }

    return cXY;
}

/////////////////////////////////////////////////////////////////////////////////////

const String ListHeaderItem::CLASS_NAME = ZGUI_LISTHEADERITEM;

ListHeaderItem::ListHeaderItem() :
_bDragable(true),
_uButtonState(0),
_iSepWidth(4),
_dwSepColor(0),
_uTextStyle(DT_VCENTER | DT_CENTER | DT_SINGLELINE),
_dwTextColor(0),
_iFont(-1),
_bShowHtml(false)
{
	SetTextPadding(Rect(2, 0, 2, 0));
    ptLastMouse.x = ptLastMouse.y = 0;
    SetMinWidth(16);
}

const String& ListHeaderItem::getClass() const
{
    return CLASS_NAME;
}

LPVOID ListHeaderItem::getInterface(const String& name)
{
	if (name == ZGUI_LISTHEADERITEM) {
		return this;
	}
    return Control::getInterface(name);
}

UINT ListHeaderItem::GetControlFlags() const
{
	if (IsEnabled() && _iSepWidth != 0) {
		return UIFLAG_SETCURSOR;
	}
	else {
		return 0;
	}
}

void ListHeaderItem::SetEnabled(bool bEnable)
{
    Control::SetEnabled(bEnable);
    if (!IsEnabled()) {
        _uButtonState = 0;
    }
}

bool ListHeaderItem::IsDragable() const
{
	return _bDragable;
}

void ListHeaderItem::SetDragable(bool bDragable)
{
    _bDragable = bDragable;
	if (!_bDragable) {
		_uButtonState &= ~UISTATE_CAPTURED;
	}
}

DWORD ListHeaderItem::GetSepWidth() const
{
	return _iSepWidth;
}

void ListHeaderItem::SetSepWidth(int iWidth)
{
    _iSepWidth = iWidth;
}

DWORD ListHeaderItem::GetTextStyle() const
{
	return _uTextStyle;
}

void ListHeaderItem::SetTextStyle(UINT uStyle)
{
    _uTextStyle = uStyle;
    Invalidate();
}

DWORD ListHeaderItem::GetTextColor() const
{
	return _dwTextColor;
}


void ListHeaderItem::SetTextColor(DWORD dwTextColor)
{
    _dwTextColor = dwTextColor;
}

RECT ListHeaderItem::GetTextPadding() const
{
	return m_rcTextPadding;
}

void ListHeaderItem::SetTextPadding(RECT rc)
{
	m_rcTextPadding = rc;
	Invalidate();
}

void ListHeaderItem::SetFont(int index)
{
    _iFont = index;
}

bool ListHeaderItem::IsShowHtml()
{
    return _bShowHtml;
}

void ListHeaderItem::SetShowHtml(bool bShowHtml)
{
    if( _bShowHtml == bShowHtml ) return;

    _bShowHtml = bShowHtml;
    Invalidate();
}

const String& ListHeaderItem::GetNormalImage() const
{
	return m_sNormalImage;
}

void ListHeaderItem::SetNormalImage(const String& pStrImage)
{
    m_sNormalImage = pStrImage;
    Invalidate();
}

const String& ListHeaderItem::GetHotImage() const
{
    return m_sHotImage;
}

void ListHeaderItem::SetHotImage(const String& pStrImage)
{
    m_sHotImage = pStrImage;
    Invalidate();
}

const String& ListHeaderItem::GetPushedImage() const
{
    return m_sPushedImage;
}

void ListHeaderItem::SetPushedImage(const String& pStrImage)
{
    m_sPushedImage = pStrImage;
    Invalidate();
}

const String& ListHeaderItem::GetFocusedImage() const
{
    return m_sFocusedImage;
}

void ListHeaderItem::SetFocusedImage(const String& pStrImage)
{
    m_sFocusedImage = pStrImage;
    Invalidate();
}

const String& ListHeaderItem::GetSepImage() const
{
    return _sSepImage;
}

void ListHeaderItem::setSepImage(const String& pStrImage)
{
    _sSepImage = pStrImage;
    Invalidate();
}

void ListHeaderItem::setSepColor(DWORD dwSepColor)
{
	_dwSepColor = dwSepColor;
	Invalidate();
}

void ListHeaderItem::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "dragable") {
        SetDragable(pstrValue == "true");
    }
    else if (pstrName == "sepwidth") {
        SetSepWidth(pstrValue.getIntValue());
    }
    else if (pstrName == "align") {
        if (pstrValue.contains("left")) {
            _uTextStyle &= ~(DT_CENTER | DT_RIGHT);
            _uTextStyle |= DT_LEFT;
        }
        if (pstrValue.contains("center")) {
            _uTextStyle &= ~(DT_LEFT | DT_RIGHT);
            _uTextStyle |= DT_CENTER;
        }
        if (pstrValue.contains("right")) {
            _uTextStyle &= ~(DT_LEFT | DT_CENTER);
            _uTextStyle |= DT_RIGHT;
        }
    }
    else if (pstrName == "endellipsis") {
        if (pstrValue == "true") {
            _uTextStyle |= DT_END_ELLIPSIS;
        }
        else {
            _uTextStyle &= ~DT_END_ELLIPSIS;
        }
    }    
    else if (pstrName == "font") {
        SetFont(pstrValue.getIntValue());
    }
    else if (pstrName == "textcolor") {
        SetTextColor((uint32_t)pstrValue.getHexValue32());
    }
	else if (pstrName == "textpadding") {
		RECT rcTextPadding;
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
        setSepImage(pstrValue);
    }
	else if (pstrName == "sepcolor") {
		setSepColor((uint32_t)pstrValue.getHexValue32());
	}
    else {
        Control::setAttribute(pstrName, pstrValue);
    }
}

void ListHeaderItem::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
		if (_pParent != 0) {
			_pParent->DoEvent(event);
		}
		else {
			Control::DoEvent(event);
		}
        return;
    }

    if (event.Type == UIEVENT_SETFOCUS) {
        Invalidate();
    }
    if (event.Type == UIEVENT_KILLFOCUS) {
        Invalidate();
    }
    if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK) {
		if (!IsEnabled()) {
			return;
		}
        RECT rcSeparator = GetThumbRect();
		if (_iSepWidth >= 0) {
			rcSeparator.left -= 4;
		}
		else {
			rcSeparator.right += 4;
		}
        if (fn_PtInRect(&rcSeparator, event.ptMouse)) {
            if (_bDragable) {
                _uButtonState |= UISTATE_CAPTURED;
                ptLastMouse = event.ptMouse;
            }
        }
        else {
            _uButtonState |= UISTATE_PUSHED;
            _pManager->SendNotify(this, ZGUI_MSGTYPE_HEADERCLICK);
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_BUTTONUP) {
        if ((_uButtonState & UISTATE_CAPTURED) != 0) {
            _uButtonState &= ~UISTATE_CAPTURED;
			if (GetParent()) { 
                GetParent()->NeedParentUpdate();
			}
        }
        else if ((_uButtonState & UISTATE_PUSHED) != 0) {
            _uButtonState &= ~UISTATE_PUSHED;
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_MOUSEMOVE) {
        if ((_uButtonState & UISTATE_CAPTURED) != 0) {
            RECT rc = _rcItem;
            if (_iSepWidth >= 0) {
                rc.right -= ptLastMouse.x - event.ptMouse.x;
            }
            else {
                rc.left -= ptLastMouse.x - event.ptMouse.x;
            }
            
            if (rc.right - rc.left > GetMinWidth()) {
                _cxyFixed.cx = rc.right - rc.left;
                ptLastMouse = event.ptMouse;
				if (GetParent()) {
                    GetParent()->NeedParentUpdate();
				}
            }
        }
        return;
    }
    if (event.Type == UIEVENT_SETCURSOR) {
        RECT rcSeparator = GetThumbRect();
		if (_iSepWidth>=0) {
			rcSeparator.left -= 4;
		}
		else {
			rcSeparator.right += 4;
		}
        if (IsEnabled() && _bDragable && fn_PtInRect(&rcSeparator, event.ptMouse)) {
            fn_SetCursor(fn_LoadCursorW(NULL, MAKEINTRESOURCE(IDC_SIZEWE)));
            return;
        }
    }
    if (event.Type == UIEVENT_MOUSEENTER) {
        if (IsEnabled()) {
            _uButtonState |= UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_MOUSELEAVE) {
        if (IsEnabled()) {
            _uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    Control::DoEvent(event);
}

SIZE ListHeaderItem::EstimateSize(SIZE szAvailable)
{
	if (_cxyFixed.cy == 0) {
		return Size(_cxyFixed.cx, _pManager->GetDefaultFontInfo()->tm.tmHeight + 14);
	}
    return Control::EstimateSize(szAvailable);
}

RECT ListHeaderItem::GetThumbRect() const
{
	if (_iSepWidth >= 0) {
		return Rect(_rcItem.right - _iSepWidth, _rcItem.top, _rcItem.right, _rcItem.bottom);
	}
	else {
		return Rect(_rcItem.left, _rcItem.top, _rcItem.left - _iSepWidth, _rcItem.bottom);
	}
}

void ListHeaderItem::PaintStatusImage(HDC hDC)
{
    if (IsFocused()) {
        _uButtonState |= UISTATE_FOCUSED;
    }
    else {
        _uButtonState &= ~ UISTATE_FOCUSED;
    }

    if ((_uButtonState & UISTATE_PUSHED) != 0) {
        if (m_sPushedImage.isEmpty() && !m_sNormalImage.isEmpty()) {
            DrawImage(hDC, m_sNormalImage);
        }
        if (!DrawImage(hDC, m_sPushedImage)) {
            m_sPushedImage = String::empty;
        }
    }
    else if ((_uButtonState & UISTATE_HOT) != 0) {
        if (m_sHotImage.isEmpty() && !m_sNormalImage.isEmpty()) {
            DrawImage(hDC, m_sNormalImage);
        }
        if (!DrawImage(hDC, m_sHotImage)) {
            m_sHotImage = String::empty;
        }
    }
    else if ((_uButtonState & UISTATE_FOCUSED) != 0) {
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

    if (!_sSepImage.isEmpty()) {
        RECT rcThumb = GetThumbRect();
        rcThumb.left -= _rcItem.left;
        rcThumb.top -= _rcItem.top;
        rcThumb.right -= _rcItem.left;
        rcThumb.bottom -= _rcItem.top;

        m_sSepImageModify = String::empty;
        m_sSepImageModify << "dest='" << rcThumb.left << "," << rcThumb.top << "," << rcThumb.right << "," << rcThumb.bottom << "'";
        if (!DrawImage(hDC, _sSepImage)) {
            _sSepImage = String::empty;
        }
		else {
			DrawImage(hDC, _sSepImage, m_sSepImageModify);
		}
    }
	else if (_dwSepColor != 0) {
		RECT rcThumb = GetThumbRect();
// 		rcThumb.left -= _rcItem.left;
// 		rcThumb.top -= _rcItem.top;
// 		rcThumb.right -= _rcItem.left;
// 		rcThumb.bottom -= _rcItem.top;
		RenderEngine::DrawColor(hDC, rcThumb, GetAdjustColor(_dwSepColor));
	}
}

void ListHeaderItem::PaintText(HDC hDC)
{
    if (_dwTextColor == 0) {
        _dwTextColor = _pManager->GetDefaultFontColor();
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
    if (_bShowHtml) {
        RenderEngine::drawHtmlText(hDC, _pManager, rcText, _text, _dwTextColor, 0, 0, nLinks, DT_SINGLELINE | _uTextStyle);
    }
    else {
        RenderEngine::DrawText(hDC, _pManager, rcText, _text, _dwTextColor, _iFont, DT_SINGLELINE | _uTextStyle);
    }
}

/////////////////////////////////////////////////////////////////////////////////////

const String ListElement::CLASS_NAME = ZGUI_LISTELEMENT;

ListElement::ListElement() : 
m_iIndex(-1),
_pOwner(NULL), 
m_bSelected(false),
m_uButtonState(0)
{
}

const String& ListElement::getClass() const
{
    return CLASS_NAME;
}

UINT ListElement::GetControlFlags() const
{
    return UIFLAG_WANTRETURN;
}

LPVOID ListElement::getInterface(const String& name)
{
	if (name == ZGUI_LISTITEM) {
		return static_cast<IListItem*>(this);
	}
	if (name == ZGUI_LISTELEMENT) {
		return static_cast<ListElement*>(this);
	}
    return Control::getInterface(name);
}

IListOwner* ListElement::GetOwner()
{
    return _pOwner;
}

void ListElement::SetOwner(Control* pOwner)
{
    _pOwner = static_cast<IListOwner*>(pOwner->getInterface("IListOwner"));
}

void ListElement::SetVisible(bool bVisible)
{
    Control::SetVisible(bVisible);
    if (!IsVisible() && m_bSelected) {
        m_bSelected = false;
		if (_pOwner != 0) {
			_pOwner->SelectItem(-1);
		}
    }
}

void ListElement::SetEnabled(bool bEnable)
{
    Control::SetEnabled(bEnable);
    if (!IsEnabled()) {
        m_uButtonState = 0;
    }
}

int ListElement::GetIndex() const
{
    return m_iIndex;
}

void ListElement::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}

void ListElement::Invalidate()
{
	if (!IsVisible()) {
		return;
	}

    if (GetParent()) {
        Container* pParentContainer = static_cast<Container*>(GetParent()->getInterface("Container"));
        if (pParentContainer != 0) {
            RECT rc = pParentContainer->GetPos();
            RECT rcInset = pParentContainer->getInset();
            rc.left += rcInset.left;
            rc.top += rcInset.top;
            rc.right -= rcInset.right;
            rc.bottom -= rcInset.bottom;
            ScrollBar* pVerticalScrollBar = pParentContainer->getVerticalScrollBar();
			if (pVerticalScrollBar && pVerticalScrollBar->IsVisible()) {
				rc.right -= pVerticalScrollBar->GetFixedWidth();
			}
            ScrollBar* pHorizontalScrollBar = pParentContainer->getHorizontalScrollBar();
			if (pHorizontalScrollBar && pHorizontalScrollBar->IsVisible()) {
				rc.bottom -= pHorizontalScrollBar->GetFixedHeight();
			}

            RECT invalidateRc = _rcItem;
            if (!fn_IntersectRect(&invalidateRc, &_rcItem, &rc)) {
                return;
            }

            Control* pParent = GetParent();
            RECT rcTemp;
            RECT rcParent;
            while (pParent = pParent->GetParent()) {
                rcTemp = invalidateRc;
                rcParent = pParent->GetPos();
                if (!fn_IntersectRect(&invalidateRc, &rcTemp, &rcParent)) {
                    return;
                }
            }

			if (_pManager != 0) {
				_pManager->Invalidate(invalidateRc);
			}
        }
        else {
            Control::Invalidate();
        }
    }
    else {
        Control::Invalidate();
    }
}

bool ListElement::Activate()
{
	if (!Control::Activate()) {
		return false;
	}
	if (_pManager != 0) {
		_pManager->SendNotify(this, ZGUI_MSGTYPE_ITEMACTIVATE);
	}
    return true;
}

bool ListElement::IsSelected() const
{
    return m_bSelected;
}

bool ListElement::Select(bool bSelect)
{
	if (!IsEnabled() || bSelect == m_bSelected) {
		return false;
	}

    m_bSelected = bSelect;
	if (bSelect && _pOwner != 0) {
		_pOwner->SelectItem(m_iIndex);
	}
    Invalidate();

    return true;
}

bool ListElement::IsExpanded() const
{
    return false;
}

bool ListElement::Expand(bool /*bExpand = true*/)
{
    return false;
}

void ListElement::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
		if (_pOwner != 0) {
			_pOwner->DoEvent(event);
		}
		else {
			Control::DoEvent(event);
		}
        return;
    }

    if (event.Type == UIEVENT_DBLCLICK) {
        if (IsEnabled()) {
            Activate();
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_KEYDOWN && IsEnabled()) {
        if (event.chKey == VK_RETURN) {
            Activate();
            Invalidate();
            return;
        }
    }
    // An important twist: The list-item will send the event not to its immediate
    // parent but to the "attached" list. A list may actually embed several components
    // in its path to the item, but key-presses etc. needs to go to the actual list.
	if (_pOwner != 0) {
		_pOwner->DoEvent(event);
	}
	else {
		Control::DoEvent(event);
	}
}

void ListElement::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "selected") {
        Select();
    }
    else {
        Control::setAttribute(pstrName, pstrValue);
    }
}

void ListElement::DrawItemBk(HDC hDC, const RECT& rcItem)
{
    zgui_assert(_pOwner);
	if (_pOwner == 0) {
		return;
	}
    ListInfo* pInfo = _pOwner->GetListInfo();
    DWORD iBackColor = 0;
	if (!pInfo->bAlternateBk || m_iIndex % 2 == 0) {
		iBackColor = pInfo->dwBkColor;
	}
    if ((m_uButtonState & UISTATE_HOT) != 0 ) {
        iBackColor = pInfo->dwHotBkColor;
    }
    if (IsSelected()) {
        iBackColor = pInfo->dwSelectedBkColor;
    }
    if (!IsEnabled()) {
        iBackColor = pInfo->dwDisabledBkColor;
    }

    if (iBackColor != 0) {
        RenderEngine::DrawColor(hDC, _rcItem, GetAdjustColor(iBackColor));
    }

    if (!IsEnabled()) {
        if (!pInfo->sDisabledImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sDisabledImage)) {
                pInfo->sDisabledImage.clear();
            }
            else {
                return;
            }
        }
    }
    if (IsSelected()) {
        if (!pInfo->sSelectedImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sSelectedImage)) {
                pInfo->sSelectedImage.clear();
            }
            else {
                return;
            }
        }
    }
    if ((m_uButtonState & UISTATE_HOT) != 0) {
        if (!pInfo->sHotImage.isEmpty()) {
            if (!DrawImage(hDC, pInfo->sHotImage)) {
                pInfo->sHotImage.clear();
            }
            else {
                return;
            }
        }
    }

    if (!_bkImageName.isEmpty()) {
        if (!pInfo->bAlternateBk || m_iIndex % 2 == 0) {
            if (!DrawImage(hDC, _bkImageName)) {
                _bkImageName.clear();
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
        RenderEngine::DrawLine(hDC, rcLine, 1, GetAdjustColor(pInfo->dwLineColor));
    }
}

/////////////////////////////////////////////////////////////////////////////////////

const String ListLabelElement::CLASS_NAME = ZGUI_LISTLABELELEMENT;

ListLabelElement::ListLabelElement()
{
}

const String& ListLabelElement::getClass() const
{
    return CLASS_NAME;
}

LPVOID ListLabelElement::getInterface(const String& name)
{
	if (name == ZGUI_LISTLABELELEMENT) {
		return static_cast<ListLabelElement*>(this);
	}
    return ListElement::getInterface(name);
}

void ListLabelElement::DoEvent(TEventUI& event)
{
    if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
		if (_pOwner != 0) {
			_pOwner->DoEvent(event);
		}
		else {
			ListElement::DoEvent(event);
		}
        return;
    }

    if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN) {
        if (IsEnabled()) {
            _pManager->SendNotify(this, ZGUI_MSGTYPE_ITEMCLICK);
            Select();
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_MOUSEMOVE) {
        return;
    }
    if (event.Type == UIEVENT_BUTTONUP) {
        return;
    }
    if (event.Type == UIEVENT_MOUSEENTER) {
        if (IsEnabled()) {
            m_uButtonState |= UISTATE_HOT;
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
    ListElement::DoEvent(event);
}

SIZE ListLabelElement::EstimateSize(SIZE szAvailable)
{
	if (_pOwner == 0) {
		return Size(0, 0);
	}

    ListInfo* pInfo = _pOwner->GetListInfo();
    SIZE cXY = _cxyFixed;
    if (cXY.cy == 0 && _pManager != 0) {
        cXY.cy = _pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
        cXY.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
    }

    if (cXY.cx == 0 && _pManager != 0) {
        RECT rcText = {0, 0, 9999, cXY.cy};
        if (pInfo->bShowHtml) {
            int nLinks = 0;
            RenderEngine::drawHtmlText(_pManager->GetPaintDC(), _pManager, rcText, _text, 0, NULL, NULL, nLinks, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
        }
        else {
            RenderEngine::DrawText(_pManager->GetPaintDC(), _pManager, rcText, _text, 0, pInfo->nFont, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
        }
        cXY.cx = rcText.right - rcText.left + pInfo->rcTextPadding.left + pInfo->rcTextPadding.right;        
    }

    return cXY;
}

void ListLabelElement::DoPaint(HDC hDC, const RECT& rcPaint)
{
	if (!fn_IntersectRect(&_rcPaint, &rcPaint, &_rcItem)) {
		return;
	}
    DrawItemBk(hDC, _rcItem);
    DrawItemText(hDC, _rcItem);
}

void ListLabelElement::DrawItemText(HDC hDC, const RECT& rcItem)
{
	if (_text.isEmpty() || _pOwner == 0) {
        return;
    }

    ListInfo* pInfo = _pOwner->GetListInfo();
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
        RenderEngine::drawHtmlText(hDC, _pManager, rcText, _text, iTextColor, NULL, NULL, nLinks, DT_SINGLELINE | pInfo->uTextStyle);
    }
    else {
        RenderEngine::DrawText(hDC, _pManager, rcText, _text, iTextColor, pInfo->nFont, DT_SINGLELINE | pInfo->uTextStyle);
    }
}


/////////////////////////////////////////////////////////////////////////////////////

const String ListTextElement::CLASS_NAME = ZGUI_LISTTEXTELEMENT;

ListTextElement::ListTextElement() :
m_nLinks(0),
m_nHoverLink(-1),
_pOwner(0)
{
    __stosb((uint8_t*)&m_rcLinks, 0, sizeof(m_rcLinks));
}

ListTextElement::~ListTextElement()
{
    String* pText;
    for (int it = 0; it < _texts.size(); ++it) {
        pText = _texts.getUnchecked(it);
        if (pText != NULL) {
            delete pText;
        }
    }
	m_uTextsStyle.clear();
    _texts.clear();
}

const String& ListTextElement::getClass() const
{
    return CLASS_NAME;
}

LPVOID ListTextElement::getInterface(const String& name)
{
	if (name == ZGUI_LISTTEXTELEMENT) {
		return static_cast<ListTextElement*>(this);
	}
    return ListLabelElement::getInterface(name);
}

UINT ListTextElement::GetControlFlags() const
{
    return UIFLAG_WANTRETURN | ( (IsEnabled() && m_nLinks > 0) ? UIFLAG_SETCURSOR : 0);
}

const String& ListTextElement::getText(int iIndex) const
{
    String* pText = _texts[iIndex];
    if (pText != 0) {
        return *pText;
    }
    return String::empty;
}

void ListTextElement::setText(int iIndex, const String& pstrText, uint32_t color, int uTextStyle)
{
    if (_pOwner == 0) {
        return;
    }
    ListInfo* pInfo = _pOwner->GetListInfo();
    if (iIndex < 0 || iIndex >= pInfo->nColumns) {
        return;
    }
    while (_texts.size() < pInfo->nColumns) {
        _texts.add(NULL);
    }

    String* pText = _texts.getUnchecked(iIndex);
    if (pText != 0 && *pText == pstrText) {
        return;
    }

    if (pText != 0) {
		*pText = pstrText;
    }
    else {
		_texts.set(iIndex, new String(pstrText));
    }

	m_uTextsStyle.insert(iIndex, uTextStyle);
    Invalidate();
}

void ListTextElement::SetOwner(Control* pOwner)
{
    ListElement::SetOwner(pOwner);
    _pOwner = static_cast<IList*>(pOwner->getInterface("IList"));
}

String* ListTextElement::GetLinkContent(int iIndex)
{
    if (iIndex >= 0 && iIndex < m_nLinks) {
        return &m_sLinks[iIndex];
    }
    return NULL;
}

void ListTextElement::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( _pOwner != NULL ) _pOwner->DoEvent(event);
        else ListLabelElement::DoEvent(event);
        return;
    }

    // When you hover over a link
    if (event.Type == UIEVENT_SETCURSOR) {
        for (int i = 0; i < m_nLinks; ++i) {
            if (fn_PtInRect(&m_rcLinks[i], event.ptMouse)) {
                fn_SetCursor(fn_LoadCursorW(NULL, MAKEINTRESOURCE(IDC_HAND)));
                return;
            }
        }      
    }
    if (event.Type == UIEVENT_BUTTONUP && IsEnabled()) {
        for (int i = 0; i < m_nLinks; ++i) {
            if (fn_PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                _pManager->SendNotify(this, ZGUI_MSGTYPE_LINK, i);
                return;
            }
        }
    }
    if (m_nLinks > 0 && event.Type == UIEVENT_MOUSEMOVE) {
        int nHoverLink = -1;
        for (int i = 0; i < m_nLinks; ++i) {
            if (fn_PtInRect(&m_rcLinks[i], event.ptMouse)) {
                nHoverLink = i;
                break;
            }
        }

        if (m_nHoverLink != nHoverLink) {
            Invalidate();
            m_nHoverLink = nHoverLink;
        }
    }
    if (m_nLinks > 0 && event.Type == UIEVENT_MOUSELEAVE) {
        if (m_nHoverLink != -1) {
            Invalidate();
            m_nHoverLink = -1;
        }
    }

	if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN) {
		if (IsEnabled()) {
			_pManager->SendNotify(this, "itemclick");
			if (_pOwner) {
				if (_pOwner->GetSingleSelect()) {
					if (!IsSelected()) {
						_pOwner->SelectItem(m_iIndex);
					}
				}
				else {
					if ((fn_GetKeyState(VK_CONTROL) & 0x8000)) {
						if (IsSelected()) {
							_pOwner->UnSelectItem(m_iIndex);
						}
						else {
							_pOwner->SelectMultiItem(m_iIndex);
						}
					}
					else {
						if ((_pOwner->GetSelectItemCount() == 1 && IsSelected())) {
						}
						else {
							_pOwner->SelectItem(m_iIndex);
						}
					}
				}
			}
			Invalidate();
		}
		_pOwner->DoEvent(event);
		return;
	}
    ListLabelElement::DoEvent(event);
}

SIZE ListTextElement::EstimateSize(SIZE szAvailable)
{
    ListInfo* pInfo = 0;
	if (_pOwner) {
		pInfo = _pOwner->GetListInfo();
	}

    SIZE cXY = _cxyFixed;
    if (cXY.cy == 0 && _pManager != 0) {
        cXY.cy = _pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
		if (pInfo) {
			cXY.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
		}
    }

    return cXY;
}

void ListTextElement::DrawItemText(HDC hDC, const RECT& rcItem)
{
	if (_pOwner == 0) {
		return;
	}
    ListInfo* pInfo = _pOwner->GetListInfo();
    uint32_t textColor = pInfo->dwTextColor;
	uint32_t neededTextColor;

    if ((m_uButtonState & UISTATE_HOT) != 0) {
		textColor = pInfo->dwHotTextColor;
    }
    if (IsSelected()) {
		textColor = pInfo->dwSelectedTextColor;
    }
    if (!IsEnabled()) {
		textColor = pInfo->dwDisabledTextColor;
    }
    IListCallback* pCallback = _pOwner->GetTextCallback();
    
    m_nLinks = 0;
	int vLineColumns = pInfo->nColumns - 1;
    int nLinks = lengthof(m_rcLinks);
    for (int i = 0; i < pInfo->nColumns; ++i) {
		int nTextAlgin = m_uTextsStyle[i];
		if (nTextAlgin < 0) {
			nTextAlgin = DT_SINGLELINE | pInfo->uTextStyle;
		}
        RECT rcItem = { pInfo->rcColumn[i].left, _rcItem.top, pInfo->rcColumn[i].right, _rcItem.bottom };
		RECT rcItemLine = rcItem;
        rcItem.left += pInfo->rcTextPadding.left;
        rcItem.right -= pInfo->rcTextPadding.right;
        rcItem.top += pInfo->rcTextPadding.top;
        rcItem.bottom -= pInfo->rcTextPadding.bottom;

		neededTextColor = textColor;

        String strText;
        if (pCallback) {
			strText = pCallback->GetItemText(this, m_iIndex, i, &neededTextColor);
        }
        else {
            strText = getText(i);
        }
        if (pInfo->bShowHtml) {
			RenderEngine::drawHtmlText(hDC, _pManager, rcItem, strText, neededTextColor, &m_rcLinks[m_nLinks], &m_sLinks[m_nLinks], nLinks, nTextAlgin);
        }
        else {
			RenderEngine::DrawText(hDC, _pManager, rcItem, strText, neededTextColor, pInfo->nFont, nTextAlgin);
        }

        m_nLinks += nLinks;
        nLinks = lengthof(m_rcLinks) - m_nLinks;

		if (pInfo->dwLineColor != 0 && pInfo->bShowVLine && i < vLineColumns){
			RECT nRc;
			nRc.left = rcItemLine.right - 1;
			nRc.top = rcItemLine.top;
			nRc.right = rcItemLine.right - 1;
			nRc.bottom = rcItemLine.bottom;

			RenderEngine::DrawLine(hDC, nRc, 1, pInfo->dwLineColor);
		}
	}
    for (int i = m_nLinks; i < lengthof(m_rcLinks); ++i) {
        __stosb((uint8_t*)(m_rcLinks + i), 0, sizeof(RECT));
        ((String*)(m_sLinks + i))->clear();
    }
}

bool ListTextElement::Select(bool bSelect /*= true*/)
{
	if (!IsEnabled()) {
		return false;
	}
	if (bSelect == m_bSelected) {
		return true;
	}

	m_bSelected = bSelect;
	Invalidate();

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////

const String ListContainerElement::CLASS_NAME = ZGUI_LISTCONTAINERELEMENT;

ListContainerElement::ListContainerElement() : 
m_iIndex(-1),
_pOwner(NULL), 
m_bSelected(false),
m_uButtonState(0)
{
}

const String& ListContainerElement::getClass() const
{
    return CLASS_NAME;
}

UINT ListContainerElement::GetControlFlags() const
{
    return UIFLAG_WANTRETURN;
}

LPVOID ListContainerElement::getInterface(const String& name)
{
	if (name == ZGUI_LISTITEM) {
		return static_cast<IListItem*>(this);
	}
	if (name == ZGUI_LISTCONTAINERELEMENT) {
		return static_cast<ListContainerElement*>(this);
	}
    return Container::getInterface(name);
}

IListOwner* ListContainerElement::GetOwner()
{
    return _pOwner;
}

void ListContainerElement::SetOwner(Control* pOwner)
{
    _pOwner = static_cast<IListOwner*>(pOwner->getInterface("IListOwner"));
}

void ListContainerElement::SetVisible(bool bVisible)
{
    Container::SetVisible(bVisible);
    if (!IsVisible() && m_bSelected) {
        m_bSelected = false;
		if (_pOwner != 0) {
			_pOwner->SelectItem(-1);
		}
    }
}

void ListContainerElement::SetEnabled(bool bEnable)
{
    Control::SetEnabled(bEnable);
    if (!IsEnabled()) {
        m_uButtonState = 0;
    }
}

int ListContainerElement::GetIndex() const
{
    return m_iIndex;
}

void ListContainerElement::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}

void ListContainerElement::Invalidate()
{
	if (!IsVisible()) {
		return;
	}

    if (GetParent() != 0) {
        Container* pParentContainer = static_cast<Container*>(GetParent()->getInterface("Container"));
        if (pParentContainer != 0) {
            RECT rc = pParentContainer->GetPos();
            RECT rcInset = pParentContainer->getInset();
            rc.left += rcInset.left;
            rc.top += rcInset.top;
            rc.right -= rcInset.right;
            rc.bottom -= rcInset.bottom;
            ScrollBar* pVerticalScrollBar = pParentContainer->getVerticalScrollBar();
            if( pVerticalScrollBar && pVerticalScrollBar->IsVisible() ) rc.right -= pVerticalScrollBar->GetFixedWidth();
            ScrollBar* pHorizontalScrollBar = pParentContainer->getHorizontalScrollBar();
            if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rc.bottom -= pHorizontalScrollBar->GetFixedHeight();

            RECT invalidateRc = _rcItem;
            if (!fn_IntersectRect(&invalidateRc, &_rcItem, &rc)) {
                return;
            }

            Control* pParent = GetParent();
            RECT rcTemp;
            RECT rcParent;
            while (pParent = pParent->GetParent()) {
                rcTemp = invalidateRc;
                rcParent = pParent->GetPos();
                if (!fn_IntersectRect(&invalidateRc, &rcTemp, &rcParent)) {
                    return;
                }
            }

			if (_pManager != 0) {
				_pManager->Invalidate(invalidateRc);
			}
        }
        else {
            Container::Invalidate();
        }
    }
    else {
        Container::Invalidate();
    }
}

bool ListContainerElement::Activate()
{
	if (!Container::Activate()) {
		return false;
	}
	if (_pManager != 0) {
		_pManager->SendNotify(this, ZGUI_MSGTYPE_ITEMACTIVATE);
	}
    return true;
}

bool ListContainerElement::IsSelected() const
{
    return m_bSelected;
}

bool ListContainerElement::Select(bool bSelect)
{
	if (!IsEnabled()) {
		return false;
	}
	if (bSelect == m_bSelected) {
		return true;
	}
    m_bSelected = bSelect;
	if (bSelect && _pOwner != 0) {
		_pOwner->SelectItem(m_iIndex);
	}
    Invalidate();

    return true;
}

bool ListContainerElement::IsExpanded() const
{
    return false;
}

bool ListContainerElement::Expand(bool /*bExpand = true*/)
{
    return false;
}

void ListContainerElement::DoEvent(TEventUI& event)
{
    if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
		if (_pOwner != 0) {
			_pOwner->DoEvent(event);
		}
		else {
			Container::DoEvent(event);
		}
        return;
    }

    if (event.Type == UIEVENT_DBLCLICK) {
        if (IsEnabled()) {
            Activate();
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_KEYDOWN && IsEnabled()) {
        if (event.chKey == VK_RETURN) {
            Activate();
            Invalidate();
            return;
        }
    }
    if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN) {
        if (IsEnabled()) {
            _pManager->SendNotify(this, ZGUI_MSGTYPE_ITEMCLICK);
            Select();
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_BUTTONUP) {
        return;
    }
    if (event.Type == UIEVENT_MOUSEMOVE) {
        return;
    }
    if (event.Type == UIEVENT_MOUSEENTER) {
        if (IsEnabled()) {
			_pManager->SendNotify(this, ZGUI_MSGTYPE_MOUSEENTER);
            m_uButtonState |= UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    if (event.Type == UIEVENT_MOUSELEAVE) {
        if ((m_uButtonState & UISTATE_HOT) != 0) {
			_pManager->SendNotify(this, ZGUI_MSGTYPE_MOUSELEAVE);
            m_uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }

    // An important twist: The list-item will send the event not to its immediate
    // parent but to the "attached" list. A list may actually embed several components
    // in its path to the item, but key-presses etc. needs to go to the actual list.
	if (_pOwner != 0) {
		_pOwner->DoEvent(event);
	}
	else {
		Control::DoEvent(event);
	}
}

void ListContainerElement::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "selected") {
        Select();
    }
    else {
        Container::setAttribute(pstrName, pstrValue);
    }
}

void ListContainerElement::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if (!fn_IntersectRect(&_rcPaint, &rcPaint, &_rcItem)) {
        return;
    }
    DrawItemBk(hDC, _rcItem);
    Container::DoPaint(hDC, rcPaint);
}

void ListContainerElement::SetPos(RECT rc)
{
	HorizontalLayout::SetPos(rc);

	if (_pOwner == 0) {
		return;
	}
	ListInfo* pInfo = _pOwner->GetListInfo();

	for (int i = 0; i < pInfo->nColumns; ++i) {
		RECT rcItem = { pInfo->rcColumn[i].left, _rcItem.top, pInfo->rcColumn[i].right, _rcItem.bottom };
		Control* pControl = _items.getUnchecked(i);
		pControl->SetPos(rcItem);
	}
}

void ListContainerElement::DrawItemText(HDC hDC, const RECT& rcItem)
{
    return;
}

void ListContainerElement::DrawItemBk(HDC hDC, const RECT& rcItem)
{
    zgui_assert(_pOwner);
	
	if (_pOwner == 0) {
		return;
	}

    ListInfo* pInfo = _pOwner->GetListInfo();
    DWORD iBackColor = 0;
	
	if (!pInfo->bAlternateBk || m_iIndex % 2 == 0) {
		iBackColor = pInfo->dwBkColor;
	}

    if ((m_uButtonState & UISTATE_HOT) != 0) {
        iBackColor = pInfo->dwHotBkColor;
    }
    if (IsSelected()) {
        iBackColor = pInfo->dwSelectedBkColor;
    }
    if (!IsEnabled()) {
        iBackColor = pInfo->dwDisabledBkColor;
    }
    if (iBackColor != 0) {
        RenderEngine::DrawColor(hDC, _rcItem, GetAdjustColor(iBackColor));
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
        RenderEngine::DrawLine(hDC, rcLine, 1, GetAdjustColor(pInfo->dwLineColor));
    }
}

} // namespace zgui

#endif // ZGUI_USE_LIST
