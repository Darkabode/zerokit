#include "zgui.h"

#ifdef ZGUI_USE_SCROLLBAR

#ifdef ZGUI_USE_CONTAINER

namespace zgui {
	
const String Container::CLASS_NAME = ZGUI_CONTAINER;


Container::Container() :
m_iChildPadding(0),
m_bAutoDestroy(true),
m_bDelayedDestroy(true),
m_bMouseChildEnabled(true),
_pVerticalScrollBar(NULL),
_pHorizontalScrollBar(NULL),
m_bScrollProcess(false),
_bNoInsetVScroll(false),
_bNoInsetHScroll(false)
{
	__stosb((uint8_t*)&_rcInset, 0, sizeof(_rcInset));
}

Container::~Container()
{
	m_bDelayedDestroy = false;
	removeAll();
    if (_pVerticalScrollBar != NULL) {
        delete _pVerticalScrollBar;
    }
    if (_pHorizontalScrollBar != NULL) {
        delete _pHorizontalScrollBar;
    }
}

const String& Container::getClass() const
{
	return CLASS_NAME;
}

LPVOID Container::getInterface(const String& name)
{
    if (name == "IContainer") {
        return static_cast<IContainer*>(this);
    }
    else if (name == ZGUI_CONTAINER) {
        return static_cast<Container*>(this);
    }
	return Control::getInterface(name);
}

Control* Container::getItem(int iIndex) const
{
	return _items[iIndex];
}

int Container::indexOf(Control* pControl) const
{
	for (int it = 0; it < _items.size(); ++it) {
		if (_items.getUnchecked(it) == pControl) {
			return it;
		}
	}

	return -1;
}

bool Container::setItem(int iIndex, Control* pControl)
{
	for (int it = 0; it < _items.size(); ++it) {
		if (_items.getUnchecked(it) == pControl) {
			NeedUpdate();            
			_items.remove(it);
			_items.insert(iIndex, pControl);
			return true;
		}
	}

	return false;
}

int Container::getCount() const
{
	return _items.size();
}

bool Container::add(Control* pControl)
{
    if (pControl == NULL) {
        return false;
    }

    if (_pManager != NULL) {
        _pManager->initControls(pControl, this);
    }
		
    if (IsVisible()) {
        NeedUpdate();
    }
    else {
        pControl->SetInternVisible(false);
    }

	_items.add(pControl);
	return true;
}

bool Container::insert(int iIndex, Control* pControl)
{
    if (pControl == NULL) {
        return false;
    }

    if (_pManager != NULL) {
        _pManager->initControls(pControl, this);
    }
    if (IsVisible()) {
        NeedUpdate();
    }
    else {
        pControl->SetInternVisible(false);
    }

	_items.insert(iIndex, pControl);
	return true;
}

bool Container::remove(Control* pControl)
{
	if (pControl == 0) {
		return false;
	}

	for (int it = 0; it < _items.size(); ++it) {
		if (_items.getUnchecked(it) == pControl) {
			NeedUpdate();
			if (m_bAutoDestroy) {
                if (m_bDelayedDestroy && _pManager) {
                    _pManager->AddDelayedCleanup(pControl);
                }
                else {
                    delete pControl;
                }
			}
			return (_items.remove(it) != 0);
		}
	}
	return false;
}

bool Container::removeAt(int iIndex)
{
	Control* pControl = getItem(iIndex);
	if (pControl != NULL) {
		return Container::remove(pControl);
	}

	return false;
}

void Container::removeAll()
{
	for (int it = 0; m_bAutoDestroy && it < _items.size(); ++it) {
        if (m_bDelayedDestroy && _pManager != 0) {
            _pManager->AddDelayedCleanup(_items.getUnchecked(it));
        }
        else {
			delete _items.getUnchecked(it);
        }
	}
	_items.clear();
	NeedUpdate();
}

bool Container::IsAutoDestroy() const
{
	return m_bAutoDestroy;
}

void Container::SetAutoDestroy(bool bAuto)
{
	m_bAutoDestroy = bAuto;
}

bool Container::IsDelayedDestroy() const
{
	return m_bDelayedDestroy;
}

void Container::SetDelayedDestroy(bool bDelayed)
{
	m_bDelayedDestroy = bDelayed;
}

RECT Container::getInset() const
{
	return _rcInset;
}

void Container::setInset(RECT& rcInset)
{
	_rcInset = rcInset;
	NeedUpdate();
}

int Container::getChildPadding() const
{
	return m_iChildPadding;
}

void Container::setChildPadding(int iPadding)
{
	m_iChildPadding = iPadding;
	NeedUpdate();
}

bool Container::IsMouseChildEnabled() const
{
	return m_bMouseChildEnabled;
}

void Container::SetMouseChildEnabled(bool bEnable)
{
	m_bMouseChildEnabled = bEnable;
}

void Container::SetVisible(bool bVisible)
{
    if (m_bVisible == bVisible) {
        return;
    }
	Control::SetVisible(bVisible);
	for (int it = 0; it < _items.size(); ++it) {
		_items.getUnchecked(it)->SetInternVisible(IsVisible());
	}
}

void Container::SetEnabled(bool bEnable)
{
	if (bEnable == m_bEnabled) {
		return;
	}
	Control::SetEnabled(bEnable);
	for (int it = 0; it < _items.size(); ++it) {
		_items.getUnchecked(it)->SetEnabled(bEnable);
	}
}

void Container::SetInternVisible(bool bVisible)
{
	Control::SetInternVisible(bVisible);
    if (_items.size() == 0) {
        return;
    }
	for (int it = 0; it < _items.size(); ++it) {
		_items.getUnchecked(it)->SetInternVisible(IsVisible());
	}
}

void Container::SetMouseEnabled(bool bEnabled)
{
    if (_pVerticalScrollBar != NULL) {
        _pVerticalScrollBar->SetMouseEnabled(bEnabled);
    }
    if (_pHorizontalScrollBar != NULL) {
        _pHorizontalScrollBar->SetMouseEnabled(bEnabled);
    }
	Control::SetMouseEnabled(bEnabled);
}

void Container::updateText()
{
	Control::updateText();
	for (int it = 0; it < _items.size(); ++it) {
		_items.getUnchecked(it)->updateText();
	}
}

void Container::DoEvent(TEventUI& event)
{
	if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
        if (_pParent != NULL) {
            _pParent->DoEvent(event);
        }
        else {
            Control::DoEvent(event);
        }
		return;
	}

	if (event.Type == UIEVENT_SETFOCUS) {
		m_bFocused = true;
		return;
	}
	if (event.Type == UIEVENT_KILLFOCUS) {
		m_bFocused = false;
		return;
	}
	if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible() && _pVerticalScrollBar->IsEnabled()) {
		if (event.Type == UIEVENT_KEYDOWN) {
			switch (event.chKey) {
			    case VK_DOWN:
				    lineDown();
				    return;
			    case VK_UP:
				    lineUp();
				    return;
			    case VK_NEXT:
				    pageDown();
				    return;
			    case VK_PRIOR:
				    pageUp();
				    return;
			    case VK_HOME:
				    homeUp();
				    return;
			    case VK_END:
				    endDown();
				    return;
			}
		}
		else if (event.Type == UIEVENT_SCROLLWHEEL) {
			switch (LOWORD(event.wParam)) {
			    case SB_LINEUP:
				    lineUp();
				    return;
			    case SB_LINEDOWN:
				    lineDown();
				    return;
			}
		}
	}
	else if (_pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible() && _pHorizontalScrollBar->IsEnabled()) {
		if (event.Type == UIEVENT_KEYDOWN) {
			switch( event.chKey ) {
			    case VK_DOWN:
				    lineRight();
				    return;
			    case VK_UP:
				    lineLeft();
				    return;
			    case VK_NEXT:
				    pageRight();
				    return;
			    case VK_PRIOR:
				    pageLeft();
				    return;
			    case VK_HOME:
				    homeLeft();
				    return;
			    case VK_END:
				    endRight();
				    return;
			}
		}
		else if (event.Type == UIEVENT_SCROLLWHEEL) {
			switch (LOWORD(event.wParam)) {
			    case SB_LINEUP:
				    lineLeft();
				    return;
			    case SB_LINEDOWN:
				    lineRight();
				    return;
			}
		}
	}
	Control::DoEvent(event);
}

SIZE Container::getScrollPos() const
{
	SIZE sz;
    __stosb((uint8_t*)&sz, 0, sizeof(sz));
    if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible()) {
        sz.cy = _pVerticalScrollBar->GetScrollPos();
    }
    if (_pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible()) {
        sz.cx = _pHorizontalScrollBar->GetScrollPos();
    }

	return sz;
}

SIZE Container::getScrollRange() const
{
	SIZE sz;
    __stosb((uint8_t*)&sz, 0, sizeof(sz));
    if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible()) {
        sz.cy = _pVerticalScrollBar->GetScrollRange();
    }
    if (_pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible()) {
        sz.cx = _pHorizontalScrollBar->GetScrollRange();
    }
	return sz;
}

void Container::setScrollPos(SIZE szPos)
{
	int cx = 0;
	int cy = 0;
	if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible()) {
		int iLastScrollPos = _pVerticalScrollBar->GetScrollPos();
		_pVerticalScrollBar->SetScrollPos(szPos.cy);
		cy = _pVerticalScrollBar->GetScrollPos() - iLastScrollPos;
	}

	if (_pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible()) {
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
        if (!pControl->IsVisible()) {
            continue;
        }
        if (pControl->IsFloat()) {
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
}

void Container::lineUp()
{
	int cyLine = 8;
	if (_pManager != 0) {
		cyLine = _pManager->GetDefaultFontInfo()->tm.tmHeight + 8;
	}

	SIZE sz = getScrollPos();
	sz.cy -= cyLine;
	setScrollPos(sz);
}

void Container::lineDown()
{
	int cyLine = 8;
	if (_pManager != 0) {
		cyLine = _pManager->GetDefaultFontInfo()->tm.tmHeight + 8;
	}

	SIZE sz = getScrollPos();
	sz.cy += cyLine;
	setScrollPos(sz);
}

void Container::pageUp()
{
	SIZE sz = getScrollPos();
	int iOffset = _rcItem.bottom - _rcItem.top - _rcInset.top - _rcInset.bottom;
    if (_pHorizontalScrollBar != 0 && _pHorizontalScrollBar->IsVisible()) {
        iOffset -= _pHorizontalScrollBar->GetFixedHeight();
    }
	sz.cy -= iOffset;
	setScrollPos(sz);
}

void Container::pageDown()
{
	SIZE sz = getScrollPos();
	int iOffset = _rcItem.bottom - _rcItem.top - _rcInset.top - _rcInset.bottom;
    if (_pHorizontalScrollBar != 0 && _pHorizontalScrollBar->IsVisible()) {
        iOffset -= _pHorizontalScrollBar->GetFixedHeight();
    }
	sz.cy += iOffset;
	setScrollPos(sz);
}

void Container::homeUp()
{
	SIZE sz = getScrollPos();
	sz.cy = 0;
	setScrollPos(sz);
}

void Container::endDown()
{
	SIZE sz = getScrollPos();
	sz.cy = getScrollRange().cy;
	setScrollPos(sz);
}

void Container::lineLeft()
{
	SIZE sz = getScrollPos();
	sz.cx -= 8;
	setScrollPos(sz);
}

void Container::lineRight()
{
	SIZE sz = getScrollPos();
	sz.cx += 8;
	setScrollPos(sz);
}

void Container::pageLeft()
{
	SIZE sz = getScrollPos();
	int iOffset = _rcItem.right - _rcItem.left - _rcInset.left - _rcInset.right;
    if (_pVerticalScrollBar != 0 && _pVerticalScrollBar->IsVisible()) {
        iOffset -= _pVerticalScrollBar->GetFixedWidth();
    }
	sz.cx -= iOffset;
	setScrollPos(sz);
}

void Container::pageRight()
{
	SIZE sz = getScrollPos();
	int iOffset = _rcItem.right - _rcItem.left - _rcInset.left - _rcInset.right;
    if (_pVerticalScrollBar != 0 && _pVerticalScrollBar->IsVisible()) {
        iOffset -= _pVerticalScrollBar->GetFixedWidth();
    }
	sz.cx += iOffset;
	setScrollPos(sz);
}

void Container::homeLeft()
{
	SIZE sz = getScrollPos();
	sz.cx = 0;
	setScrollPos(sz);
}

void Container::endRight()
{
	SIZE sz = getScrollPos();
	sz.cx = getScrollRange().cx;
	setScrollPos(sz);
}

void Container::enableScrollBar(bool bEnableVertical, bool bEnableHorizontal)
{
	if (bEnableVertical && !_pVerticalScrollBar) {
		_pVerticalScrollBar = new ScrollBar;
		_pVerticalScrollBar->SetOwner(this);
		_pVerticalScrollBar->SetManager(_pManager, 0, false);
		if (_pManager != 0) {
			const String& defaultAttributes = _pManager->getDefaultAttributeList("VScrollBar");
			if (!defaultAttributes.isEmpty()) {
				_pVerticalScrollBar->applyAttributeList(defaultAttributes);
			}
		}
	}
	else if (!bEnableVertical && _pVerticalScrollBar) {
		delete _pVerticalScrollBar;
		_pVerticalScrollBar = 0;
	}

	if (bEnableHorizontal && !_pHorizontalScrollBar) {
		_pHorizontalScrollBar = new ScrollBar;
		_pHorizontalScrollBar->SetHorizontal(true);
		_pHorizontalScrollBar->SetOwner(this);
		_pHorizontalScrollBar->SetManager(_pManager, 0, false);
		if ( _pManager ) {
			const String& defaultAttributes = _pManager->getDefaultAttributeList("HScrollBar");
			if (!defaultAttributes.isEmpty()) {
				_pHorizontalScrollBar->applyAttributeList(defaultAttributes);
			}
		}
	}
	else if( !bEnableHorizontal && _pHorizontalScrollBar ) {
		delete _pHorizontalScrollBar;
		_pHorizontalScrollBar = 0;
	}

	NeedUpdate();
}

ScrollBar* Container::getVerticalScrollBar() const
{
	return _pVerticalScrollBar;
}

ScrollBar* Container::getHorizontalScrollBar() const
{
	return _pHorizontalScrollBar;
}

int Container::FindSelectable(int iIndex, bool bForward /*= true*/) const
{
	// NOTE: This is actually a helper-function for the list/combo/ect controls
	//       that allow them to find the next enabled/available selectable item
    if (getCount() == 0) {
        return -1;
    }
	iIndex = CLAMP(iIndex, 0, getCount() - 1);
	if (bForward) {
		for (int i = iIndex; i < getCount(); ++i) {
            if (getItem(i)->getInterface(ZGUI_LISTITEM) != 0 && getItem(i)->IsVisible() && getItem(i)->IsEnabled()) {
                return i;
            }
		}
		return -1;
	}
	else {
		for (int i = iIndex; i >= 0; --i) {
            if (getItem(i)->getInterface(ZGUI_LISTITEM) != 0 && getItem(i)->IsVisible() && getItem(i)->IsEnabled()) {
                return i;
            }
		}
		return FindSelectable(0, true);
	}
}

void Container::SetPos(RECT rc)
{
	Control::SetPos(rc);
	if (_items.size() == 0) {
		return;
	}
	rc.left += _rcInset.left;
	rc.top += _rcInset.top;
	rc.right -= _rcInset.right;
	rc.bottom -= _rcInset.bottom;

	for (int i = 0; i < _items.size(); ++i) {
		Control* pControl = _items.getUnchecked(i);
		if (!pControl->IsVisible()) {
			continue;
		}
		if (pControl->IsFloat()) {
			SetFloatPos(i);
		}
		else {
			pControl->SetPos(rc);
		}
	}
}

void Container::setAttribute(const String& name, const String& value)
{
	if (name == "inset") {
		RECT rcInset;
        if (Helper::splitString(value, ",", String::empty, (int&)rcInset.left, (int&)rcInset.top, (int&)rcInset.right, (int&)rcInset.bottom)) {
            setInset(rcInset);
        }
	}
	else if (name == "noinsetvscroll") {
		_bNoInsetVScroll = (value == "true");
	}
	else if (name == "noinsethscroll") {
		_bNoInsetHScroll = (value == "true");
	}
    else if (name == "mousechild") {
        SetMouseChildEnabled(value == "true");
    }
	else if (name == "vscrollbar") {
		enableScrollBar(value == "true", getHorizontalScrollBar() != NULL);
	}
	else if (name == "vscrollbarstyle") {
		enableScrollBar(true, getHorizontalScrollBar() != NULL);
        if (getVerticalScrollBar()) {
            getVerticalScrollBar()->applyAttributeList(value);
        }
	}
	else if (name == "hscrollbar") {
		enableScrollBar(getVerticalScrollBar() != NULL, value == "true");
	}
	else if (name == "hscrollbarstyle") {
		enableScrollBar(getVerticalScrollBar() != NULL, true);
        if (getHorizontalScrollBar()) {
            getHorizontalScrollBar()->applyAttributeList(value);
        }
	}
    else if (name == "childpadding") {
        setChildPadding(value.getIntValue());
    }
    else {
        Control::setAttribute(name, value);
    }
}

void Container::SetManager(PaintManager* pManager, Control* pParent, bool bInit)
{
	for (int it = 0; it < _items.size(); ++it) {
		_items.getUnchecked(it)->SetManager(pManager, this, bInit);
	}

    if (_pVerticalScrollBar != NULL) {
        _pVerticalScrollBar->SetManager(pManager, this, bInit);
    }
    if (_pHorizontalScrollBar != NULL) {
        _pHorizontalScrollBar->SetManager(pManager, this, bInit);
    }

	Control::SetManager(pManager, pParent, bInit);
}

Control* Container::FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags)
{
	// Check if this guy is valid
	if ((uFlags & UIFIND_VISIBLE) != 0 && !IsVisible()) {
		return 0;
	}
	if ((uFlags & UIFIND_ENABLED) != 0 && !IsEnabled()) {
		return 0;
	}
	if ((uFlags & UIFIND_HITTEST) != 0 ) {
		if (!fn_PtInRect(&_rcItem, *(static_cast<LPPOINT>(pData)))) {
			return 0;
		}
		if (!m_bMouseChildEnabled ) {
			Control* pResult = 0;
			if (_pVerticalScrollBar != 0) {
				pResult = _pVerticalScrollBar->FindControl(Proc, pData, uFlags);
			}
			if (pResult == 0 && _pHorizontalScrollBar != 0) {
				pResult = _pHorizontalScrollBar->FindControl(Proc, pData, uFlags);
			}
			if (pResult == 0) {
				pResult = Control::FindControl(Proc, pData, uFlags);
			}
			return pResult;
		}
	}

	Control* pResult = 0;
	if (_pVerticalScrollBar != 0) {
		pResult = _pVerticalScrollBar->FindControl(Proc, pData, uFlags);
	}
	if (pResult == 0 && _pHorizontalScrollBar != 0) {
		pResult = _pHorizontalScrollBar->FindControl(Proc, pData, uFlags);
	}
	if (pResult != 0) {
		return pResult;
	}

	if ((uFlags & UIFIND_ME_FIRST) != 0) {
		Control* pControl = Control::FindControl(Proc, pData, uFlags);
		if (pControl != 0) {
			return pControl;
		}
	}
	RECT rc = _rcItem;
	rc.left += _rcInset.left;
	rc.top += _rcInset.top;
	rc.right -= _rcInset.right;
	rc.bottom -= _rcInset.bottom;
	if (_pVerticalScrollBar && _pVerticalScrollBar->IsVisible()) {
		rc.right -= _pVerticalScrollBar->GetFixedWidth();
	}
	if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
		rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();
	}
	if ((uFlags & UIFIND_TOP_FIRST) != 0 ) {
		for (int it = _items.size() - 1; it >= 0; --it) {
			Control* pControl = _items.getUnchecked(it)->FindControl(Proc, pData, uFlags);
			if (pControl != 0) {
				if ((uFlags & UIFIND_HITTEST) != 0 && !pControl->IsFloat() && !fn_PtInRect(&rc, *(static_cast<LPPOINT>(pData)))) {
					continue;
				}
				else {
					return pControl;
				}
			}            
		}
	}
	else {
		for (int it = 0; it < _items.size(); ++it) {
			Control* pControl = _items.getUnchecked(it)->FindControl(Proc, pData, uFlags);
			if (pControl != 0) {
				if ((uFlags & UIFIND_HITTEST) != 0 && !pControl->IsFloat() && !fn_PtInRect(&rc, *(static_cast<LPPOINT>(pData)))) {
					continue;
				}
				else {
					return pControl;
				}
			} 
		}
	}

	if (pResult == 0 && (uFlags & UIFIND_ME_FIRST) == 0) {
		pResult = Control::FindControl(Proc, pData, uFlags);
	}
	return pResult;
}

void Container::DoPaint(HDC hDC, const RECT& rcPaint)
{
	RECT rcTemp;
	__stosb((uint8_t*)&rcTemp, 0, sizeof(rcTemp));
    if (!fn_IntersectRect(&rcTemp, &rcPaint, &_rcItem)) {
        return;
    }

	RenderClip clip;
	RenderClip::GenerateClip(hDC, rcTemp, clip);
	Control::DoPaint(hDC, rcPaint); 

	if (_items.size() > 0) { 
		RECT rc = _rcItem;
		rc.left += _rcInset.left;
		rc.top += _rcInset.top;  
		rc.right -= _rcInset.right; 
		rc.bottom -= _rcInset.bottom;
        if (_pVerticalScrollBar != 0&& _pVerticalScrollBar->IsVisible()) {
            rc.right -= _pVerticalScrollBar->GetFixedWidth();
        } 
        if (_pHorizontalScrollBar != 0&& _pHorizontalScrollBar->IsVisible()) {
            rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();
        }
		if (!fn_IntersectRect(&rcTemp, &rcPaint, &rc)) {
			for (int it = 0; it < _items.size(); ++it) {
				Control* pControl = _items.getUnchecked(it);
                if (!pControl->IsVisible()) { 
                    continue;
                }
                if (!fn_IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos())) {
                    continue;
                }
				if (pControl->IsFloat() ) {
                    if (!fn_IntersectRect(&rcTemp, &_rcItem, &pControl->GetPos())) {
                        continue;
                    }
					pControl->DoPaint(hDC, rcPaint);
				}
			}
		}
		else {
			RenderClip childClip;
			RenderClip::GenerateClip(hDC, rcTemp, childClip);
			for (int it = 0; it < _items.size(); ++it) {
				Control* pControl = _items.getUnchecked(it);
                if (!pControl->IsVisible()) {
                    continue;
                }
                if (!fn_IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos())) { 
                    continue;
                }
				if (pControl ->IsFloat()) {
                    if (!fn_IntersectRect(&rcTemp, &_rcItem, &pControl->GetPos())) {
                        continue;
                    }
					RenderClip::UseOldClipBegin(hDC, childClip);
					pControl->DoPaint(hDC, rcPaint);
					RenderClip::UseOldClipEnd(hDC, childClip);
				} 
				else { 
                    if (!fn_IntersectRect(&rcTemp, &rc, &pControl->GetPos())) {
                        continue;
                    }
					pControl->DoPaint(hDC, rcPaint);
				}
			} 
		}
	}

	if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible()) {
		if (fn_IntersectRect(&rcTemp, &rcPaint, &_pVerticalScrollBar->GetPos())) {
			_pVerticalScrollBar->DoPaint(hDC, rcPaint);
		}
	}

	if (_pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible()) {
		if (fn_IntersectRect(&rcTemp, &rcPaint, &_pHorizontalScrollBar->GetPos())) {
			_pHorizontalScrollBar->DoPaint(hDC, rcPaint);
		}
	}
}

void Container::SetFloatPos(int iIndex)
{
	if (iIndex < 0 || iIndex >= _items.size()) {
		return;
	}

	Control* pControl = _items.getUnchecked(iIndex);

	if (!pControl->IsVisible()) {
		return;
	}
	if (!pControl->IsFloat()) {
		return;
	}

	SIZE szXY = pControl->GetFixedXY();
	SIZE sz = {pControl->GetFixedWidth(), pControl->GetFixedHeight()};
	RECT rcCtrl;
    __stosb((uint8_t*)&rcCtrl, 0, sizeof(rcCtrl));
	if( szXY.cx >= 0 ) {
		rcCtrl.left = _rcItem.left + szXY.cx;
		rcCtrl.right = _rcItem.left + szXY.cx + sz.cx;
	}
	else {
		rcCtrl.left = _rcItem.right + szXY.cx - sz.cx;
		rcCtrl.right = _rcItem.right + szXY.cx;
	}
	if( szXY.cy >= 0 ) {
		rcCtrl.top = _rcItem.top + szXY.cy;
		rcCtrl.bottom = _rcItem.top + szXY.cy + sz.cy;
	}
	else {
		rcCtrl.top = _rcItem.bottom + szXY.cy - sz.cy;
		rcCtrl.bottom = _rcItem.bottom + szXY.cy;
	}
	if (pControl->IsRelativePos()) {
		TRelativePosUI tRelativePos = pControl->GetRelativePos();
		SIZE szParent = {_rcItem.right-_rcItem.left,_rcItem.bottom-_rcItem.top};
		if (tRelativePos.szParent.cx != 0) {
			int nIncrementX = szParent.cx-tRelativePos.szParent.cx;
			int nIncrementY = szParent.cy-tRelativePos.szParent.cy;
			rcCtrl.left += (nIncrementX*tRelativePos.nMoveXPercent/100);
			rcCtrl.top += (nIncrementY*tRelativePos.nMoveYPercent/100);
			rcCtrl.right = rcCtrl.left+sz.cx+(nIncrementX*tRelativePos.nZoomXPercent/100);
			rcCtrl.bottom = rcCtrl.top+sz.cy+(nIncrementY*tRelativePos.nZoomYPercent/100);
		}
		pControl->SetRelativeParentSize(szParent);
	}
	pControl->SetPos(rcCtrl);
}

void Container::processScrollBar(RECT rc, int cxRequired, int cyRequired)
{
	if (_pHorizontalScrollBar != 0 && _pHorizontalScrollBar->IsVisible()) {
		RECT rcScrollBarPos = {rc.left, rc.bottom, rc.right, rc.bottom + _pHorizontalScrollBar->GetFixedHeight()};
		_pHorizontalScrollBar->SetPos(rcScrollBarPos);
	}
// 
//         if (cxRequired > 0) {
//             if (_pHorizontalScrollBar == NULL) {
//                 return;
//             }
// 
//             if (cxRequired > rc.right - rc.left && !_pHorizontalScrollBar->IsVisible()) {
//                 _pHorizontalScrollBar->SetVisible(true);
//                 _pHorizontalScrollBar->SetScrollRange(cxRequired - (rc.right - rc.left));
//                 _pHorizontalScrollBar->SetScrollPos(0);
//                 m_bScrollProcess = true;
//                 SetPos(_rcItem);
//                 m_bScrollProcess = false;
//                 return;
//             }
//             // No scrollbar required
//             if (!_pHorizontalScrollBar->IsVisible()) {
//                 return;
//             }
// 
//             // Scroll not needed anymore?
//             int cxScroll = cxRequired - (rc.right - rc.left);
//             if (cxScroll <= 0 && !m_bScrollProcess) {
//                 _pHorizontalScrollBar->SetVisible(false);
//                 _pHorizontalScrollBar->SetScrollPos(0);
//                 _pHorizontalScrollBar->SetScrollRange(0);
//                 SetPos(_rcItem);
//             }
//             else {
//                 RECT rcScrollBarPos = {rc.right, rc.top, rc.right, rc.bottom + _pHorizontalScrollBar->GetFixedWidth()};
//                 _pHorizontalScrollBar->SetPos(rcScrollBarPos);
// 
//                 if (_pHorizontalScrollBar->GetScrollRange() != cxScroll) {
//                     int iScrollPos = _pHorizontalScrollBar->GetScrollPos();
//                     _pHorizontalScrollBar->SetScrollRange(utils_abs(cxScroll));
//                     if (_pHorizontalScrollBar->GetScrollRange() == 0) {
//                         _pHorizontalScrollBar->SetVisible(false);
//                         _pHorizontalScrollBar->SetScrollPos(0);
//                     }
//                     if (iScrollPos > _pHorizontalScrollBar->GetScrollPos()) {
//                         SetPos(_rcItem);
//                     }
//                 }
//             }
//         }

    if (cyRequired > 0) {
        if (_pVerticalScrollBar == NULL) {
            return;
        }

		if (cyRequired > rc.bottom - rc.top && !_pVerticalScrollBar->IsVisible()) {
			_pVerticalScrollBar->SetVisible(true);
			_pVerticalScrollBar->SetScrollRange(cyRequired - (rc.bottom - rc.top));
			_pVerticalScrollBar->SetScrollPos(0);
			m_bScrollProcess = true;
			SetPos(_rcItem);
			m_bScrollProcess = false;
			return;
		}
		// No scrollbar required
        if (!_pVerticalScrollBar->IsVisible()) {
            return;
        }

		// Scroll not needed anymore?
		int cyScroll = cyRequired - (rc.bottom - rc.top);
		if (cyScroll <= 0 && !m_bScrollProcess) {
			_pVerticalScrollBar->SetVisible(false);
			_pVerticalScrollBar->SetScrollPos(0);
			_pVerticalScrollBar->SetScrollRange(0);
			SetPos(_rcItem);
		}
		else {
			RECT rcScrollBarPos = { rc.right, rc.top, rc.right + _pVerticalScrollBar->GetFixedWidth(), rc.bottom };
			_pVerticalScrollBar->SetPos(rcScrollBarPos);

			if (_pVerticalScrollBar->GetScrollRange() != cyScroll) {
				int iScrollPos = _pVerticalScrollBar->GetScrollPos();
				_pVerticalScrollBar->SetScrollRange(fn_utils_abs(cyScroll));
				if (_pVerticalScrollBar->GetScrollRange() == 0) {
					_pVerticalScrollBar->SetVisible(false);
					_pVerticalScrollBar->SetScrollPos(0);
				}
				if (iScrollPos > _pVerticalScrollBar->GetScrollPos()) {
					SetPos(_rcItem);
				}
			}
		}
    }
}

bool Container::SetSubControlText( LPCTSTR pstrSubControlName,LPCTSTR pstrText )
{
	Control* pSubControl=NULL;
	pSubControl=this->FindSubControl(pstrSubControlName);
	if (pSubControl!=NULL)
	{
		pSubControl->setText(pstrText);
		return TRUE;
	}
	else
		return FALSE;
}

bool Container::SetSubControlFixedHeight( LPCTSTR pstrSubControlName,int cy )
{
	Control* pSubControl=NULL;
	pSubControl=this->FindSubControl(pstrSubControlName);
	if (pSubControl!=NULL)
	{
		pSubControl->SetFixedHeight(cy);
		return TRUE;
	}
	else
		return FALSE;
}

bool Container::SetSubControlFixedWdith( LPCTSTR pstrSubControlName,int cx )
{
	Control* pSubControl=NULL;
	pSubControl=this->FindSubControl(pstrSubControlName);
	if (pSubControl!=NULL)
	{
		pSubControl->SetFixedWidth(cx);
		return TRUE;
	}
	else
		return FALSE;
}

bool Container::SetSubControlUserData( LPCTSTR pstrSubControlName,LPCTSTR pstrText )
{
	Control* pSubControl=NULL;
	pSubControl=this->FindSubControl(pstrSubControlName);
	if (pSubControl!=NULL)
	{
		pSubControl->SetUserData(pstrText);
		return TRUE;
	}
	else
		return FALSE;
}

String Container::GetSubControlText(LPCTSTR pstrSubControlName)
{
	Control* pSubControl=NULL;
	pSubControl=this->FindSubControl(pstrSubControlName);
    if (pSubControl==NULL) {
		return "";
    }
    else {
		return pSubControl->getText();
    }
}

int Container::GetSubControlFixedHeight( LPCTSTR pstrSubControlName )
{
	Control* pSubControl=NULL;
	pSubControl=this->FindSubControl(pstrSubControlName);
	if (pSubControl==NULL)
		return -1;
	else
		return pSubControl->GetFixedHeight();
}

int Container::GetSubControlFixedWdith( LPCTSTR pstrSubControlName )
{
	Control* pSubControl=NULL;
	pSubControl=this->FindSubControl(pstrSubControlName);
	if (pSubControl==NULL)
		return -1;
	else
		return pSubControl->GetFixedWidth();
}

String Container::GetSubControlUserData( LPCTSTR pstrSubControlName )
{
	Control* pSubControl=NULL;
	pSubControl=this->FindSubControl(pstrSubControlName);
    if (pSubControl==NULL) {
		return "";
    }
    else {
		return pSubControl->GetUserData();
    }
}

Control* Container::FindSubControl( LPCTSTR pstrSubControlName )
{
	Control* pSubControl=NULL;
	pSubControl=static_cast<Control*>(getManager()->FindSubControlByName(this,pstrSubControlName));
	return pSubControl;
}

} // namespace zgui

#endif // ZGUI_USE_CONTAINER

#endif // ZGUI_USE_SCROLLBAR