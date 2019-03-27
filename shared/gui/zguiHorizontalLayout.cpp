#include "zgui.h"

#ifdef ZGUI_USE_CONTAINER

namespace zgui {

const String HorizontalLayout::CLASS_NAME = ZGUI_HORIZONTALLAYOUT;

HorizontalLayout::HorizontalLayout() :
_iSepWidth(0),
_uButtonState(0),
_bImmMode(false),
_childSize(false)
{
	_ptLastMouse.x = _ptLastMouse.y = 0;
	__stosb((uint8_t*)&_rcNewPos, 0, sizeof(_rcNewPos));
}

const String& HorizontalLayout::getClass() const
{
	return CLASS_NAME;
}

LPVOID HorizontalLayout::getInterface(const String& name)
{
	if (name == ZGUI_HORIZONTALLAYOUT) {
        return static_cast<HorizontalLayout*>(this);
    }
	return Container::getInterface(name);
}

UINT HorizontalLayout::GetControlFlags() const
{
    if (IsEnabled() && _iSepWidth != 0) {
        return UIFLAG_SETCURSOR;
    }
    
    return 0;
}

void HorizontalLayout::SetPos(RECT rc)
{
	Control::SetPos(rc);
	rc = _rcItem;

	// Adjust for inset
	rc.left += _rcInset.left;
	rc.top += _rcInset.top;
	rc.right -= _rcInset.right;
	rc.bottom -= _rcInset.bottom;

	if (_items.size() == 0) {
		processScrollBar(rc, 0, 0);
		return;
	}

    if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible()) {
        rc.right -= _pVerticalScrollBar->GetFixedWidth();
    }
    if (_pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible()) {
        rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();
    }

	// Determine the width of elements that are sizeable
	SIZE szAvailable = {rc.right - rc.left, rc.bottom - rc.top};
    if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
		szAvailable.cx += _pHorizontalScrollBar->GetScrollRange();
    }

	int nAdjustables = 0;
	int cxFixed = 0;
	int nEstimateNum = 0;
	for (int it1 = 0; it1 < _items.size(); ++it1) {
		Control* pControl = _items.getUnchecked(it1);
		if (!pControl->IsVisible()) {
			continue;
		}
		if (pControl->IsFloat()) {
			continue;
		}
		SIZE sz = pControl->EstimateSize(szAvailable);
		if (sz.cx == 0) {
			++nAdjustables;
		}
		else {
			if (sz.cx < pControl->GetMinWidth()) {
				sz.cx = pControl->GetMinWidth();
			}
			if (sz.cx > pControl->GetMaxWidth()) {
				sz.cx = pControl->GetMaxWidth();
			}
		}
		cxFixed += sz.cx +  pControl->GetPadding().left + pControl->GetPadding().right;
		++nEstimateNum;
	}
	cxFixed += (nEstimateNum - 1) * m_iChildPadding;

	int cxExpand = 0;
    int cxNeeded = 0;
    if (nAdjustables > 0) {
        cxExpand = MAX(0, (szAvailable.cx - cxFixed) / nAdjustables);
    }
	// Position the elements
	SIZE szRemaining = szAvailable;
	int iPosX = rc.left;
	if (_pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible()) {
		iPosX -= _pHorizontalScrollBar->GetScrollPos();
	}
    int iPosY = rc.top;
    if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible() ) {
        iPosY -= _pVerticalScrollBar->GetScrollPos();
    }
	int iAdjustable = 0;
	int cxFixedRemaining = cxFixed;
	for (int it2 = 0; it2 < _items.size(); ++it2) {
		Control* pControl = _items.getUnchecked(it2);
		if (!pControl->IsVisible()) {
			continue;
		}
		if (pControl->IsFloat()) {
			SetFloatPos(it2);
			continue;
		}
		RECT rcPadding = pControl->GetPadding();
		szRemaining.cx -= rcPadding.left;
		SIZE sz = pControl->EstimateSize(szRemaining);
		if (sz.cx == 0) {
			++iAdjustable;
			sz.cx = cxExpand;
			// Distribute remaining to last element (usually round-off left-overs)
			if (iAdjustable == nAdjustables) {
				sz.cx = MAX(0, szRemaining.cx - rcPadding.right - cxFixedRemaining);
			}
			if (sz.cx < pControl->GetMinWidth()) {
				sz.cx = pControl->GetMinWidth();
			}
			if (sz.cx > pControl->GetMaxWidth()) {
				sz.cx = pControl->GetMaxWidth();
			}
		}
		else {
			if (sz.cx < pControl->GetMinWidth()) {
				sz.cx = pControl->GetMinWidth();
			}
			if (sz.cx > pControl->GetMaxWidth()) {
				sz.cx = pControl->GetMaxWidth();
			}
			cxFixedRemaining -= sz.cx;
		}

		sz.cy = pControl->GetFixedHeight();
        if (sz.cy == 0) {
            sz.cy = szAvailable.cy - rcPadding.top - rcPadding.bottom;
        }
        if (sz.cy < 0) {
            sz.cy = 0;
        }
        if (sz.cy < pControl->GetMinHeight()) {
            sz.cy = pControl->GetMinHeight();
        }
        if (sz.cy > pControl->GetMaxHeight()) {
            sz.cy = pControl->GetMaxHeight();
        }

		//RECT rcCtrl = {iPosX + rcPadding.left, iPosY + rcPadding.top, iPosX + sz.cx + rcPadding.left + rcPadding.right, rc.top + rcPadding.top + sz.cy};
        RECT rcCtrl = {iPosX + rcPadding.left, iPosY + rcPadding.top, iPosX + rcPadding.left + sz.cx, iPosY + sz.cy + rcPadding.top + rcPadding.bottom};
		pControl->SetPos(rcCtrl);
		iPosX += sz.cx + m_iChildPadding + rcPadding.left + rcPadding.right;
        cxNeeded += sz.cx + rcPadding.left + rcPadding.right;
		szRemaining.cx -= sz.cx + m_iChildPadding + rcPadding.right;
	}
    cxNeeded += (nEstimateNum - 1) * m_iChildPadding;

	// Process the scrollbar
	if (_bNoInsetHScroll) {
		rc.bottom += _rcInset.bottom;
	}
	processScrollBar(rc, cxNeeded, 0);
}

void HorizontalLayout::DoPostPaint(HDC hDC, const RECT& /*rcPaint*/)
{
	if ((_uButtonState & UISTATE_CAPTURED) != 0 && !_bImmMode) {
		RECT rcSeparator = GetThumbRect(true);
		RenderEngine::DrawColor(hDC, rcSeparator, 0xAA000000);
	}
}

void HorizontalLayout::SetSepWidth(int iWidth)
{
	_iSepWidth = iWidth;
}

int HorizontalLayout::GetSepWidth() const
{
	return _iSepWidth;
}

void HorizontalLayout::SetSepImmMode(bool bImmediately)
{
	if( _bImmMode == bImmediately ) return;
	if( (_uButtonState & UISTATE_CAPTURED) != 0 && !_bImmMode && _pManager != NULL ) {
		_pManager->RemovePostPaint(this);
	}

	_bImmMode = bImmediately;
}

void HorizontalLayout::SetChildSize(bool childSize)
{
	_childSize = childSize;
}

bool HorizontalLayout::IsSepImmMode() const
{
	return _bImmMode;
}

void HorizontalLayout::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "sepwidth") {
        SetSepWidth(pstrValue.getIntValue());
    }
    else if (pstrName == "sepimm") {
        SetSepImmMode(pstrValue == "true");
    }
	else if (pstrName == "childsize") {
		SetChildSize(pstrValue == "true");
	}
    else {
        Container::setAttribute(pstrName, pstrValue);
    }
}

void HorizontalLayout::DoEvent(TEventUI& event)
{
	if (_iSepWidth != 0) {
		if (event.Type == UIEVENT_BUTTONDOWN && IsEnabled()) {
			RECT rcSeparator = GetThumbRect(false);
			if (fn_PtInRect(&rcSeparator, event.ptMouse)) {
				_uButtonState |= UISTATE_CAPTURED;
				_ptLastMouse = event.ptMouse;
				_rcNewPos = _rcItem;
				if( !_bImmMode && _pManager ) _pManager->AddPostPaint(this);
				return;
			}
		}
		if (event.Type == UIEVENT_BUTTONUP) {
			if( (_uButtonState & UISTATE_CAPTURED) != 0 ) {
				_uButtonState &= ~UISTATE_CAPTURED;
				_rcItem = _rcNewPos;
				if( !_bImmMode && _pManager ) _pManager->RemovePostPaint(this);
				NeedParentUpdate();
				return;
			}
		}
		if (event.Type == UIEVENT_MOUSEMOVE) {
			if ((_uButtonState & UISTATE_CAPTURED) != 0) {
				LONG cx = event.ptMouse.x - _ptLastMouse.x;
				_ptLastMouse = event.ptMouse;
				RECT rc = _rcNewPos;
				if (_iSepWidth >= 0) {
					if (cx > 0 && event.ptMouse.x < _rcNewPos.right - _iSepWidth) {
						return;
					}
					if (cx < 0 && event.ptMouse.x > _rcNewPos.right) {
						return;
					}
					rc.right += cx;
					if (rc.right - rc.left <= GetMinWidth()) {
						if (_rcNewPos.right - _rcNewPos.left <= GetMinWidth()) {
							return;
						}
						rc.right = rc.left + GetMinWidth();
					}
					if (rc.right - rc.left >= GetMaxWidth()) {
						if (_rcNewPos.right - _rcNewPos.left >= GetMaxWidth()) {
							return;
						}
						rc.right = rc.left + GetMaxWidth();
					}
				}
				else {
					if (cx > 0 && event.ptMouse.x < _rcNewPos.left) {
						return;
					}
					if (cx < 0 && event.ptMouse.x > _rcNewPos.left - _iSepWidth) {
						return;
					}
					rc.left += cx;
					if (rc.right - rc.left <= GetMinWidth()) {
						if (_rcNewPos.right - _rcNewPos.left <= GetMinWidth()) {
							return;
						}
						rc.left = rc.right - GetMinWidth();
					}
					if (rc.right - rc.left >= GetMaxWidth()) {
						if (_rcNewPos.right - _rcNewPos.left >= GetMaxWidth()) {
							return;
						}
						rc.left = rc.right - GetMaxWidth();
					}
				}

				Rect rcInvalidate = GetThumbRect(true);
				_rcNewPos = rc;
				_cxyFixed.cx = _rcNewPos.right - _rcNewPos.left;

				if (_bImmMode) {
					_rcItem = _rcNewPos;
					NeedParentUpdate();
				}
				else {
					rcInvalidate.joinWith(GetThumbRect(true));
					rcInvalidate.joinWith(GetThumbRect(false));
					if (_pManager) {
						_pManager->Invalidate(rcInvalidate);
					}
				}
				return;
			}
		}
		if (event.Type == UIEVENT_SETCURSOR) {
			RECT rcSeparator = GetThumbRect(false);
			if (IsEnabled() && fn_PtInRect(&rcSeparator, event.ptMouse)) {
				fn_SetCursor(fn_LoadCursorW(NULL, MAKEINTRESOURCE(IDC_SIZEWE)));
				return;
			}
		}
	}
	Container::DoEvent(event);
}

SIZE HorizontalLayout::EstimateSize(SIZE szAvailable)
{
	if (!_childSize) {
		return Control::EstimateSize(szAvailable);
	}

	SIZE sz, szRemaining = szAvailable;
	__stosb((uint8_t*)&sz, 0, sizeof(sz));
	//sz.cx = szAvailable.cx;
	for (int it = 0; it < _items.size(); ++it) {
		Control* pControl = _items.getUnchecked(it);
		if (!pControl->IsVisible()) {
			continue;
		}
		if (pControl->IsFloat()) {
			// Игнорируем контролы у которых задана конкретная позиция.
			continue;
		}

		SIZE ctrlSZ = pControl->EstimateSize(szRemaining);
		if (ctrlSZ.cy < pControl->GetMinHeight()) {
			ctrlSZ.cy = pControl->GetMinHeight();
		}
		if (ctrlSZ.cy > pControl->GetMaxHeight()) {
			ctrlSZ.cy = pControl->GetMaxHeight();
		}
		if (ctrlSZ.cx < pControl->GetMinWidth()) {
			ctrlSZ.cx = pControl->GetMinWidth();
		}
		if (ctrlSZ.cx > pControl->GetMaxWidth()) {
			ctrlSZ.cx = pControl->GetMaxWidth();
		}
		szRemaining.cx -= ctrlSZ.cx;
		szRemaining.cy -= ctrlSZ.cy;
		sz.cy += ctrlSZ.cy;
		sz.cx += ctrlSZ.cx;
	}

	sz.cx += _rcInset.left + _rcInset.right;
	sz.cy += _rcInset.top + _rcInset.bottom;

	return sz;
}

RECT HorizontalLayout::GetThumbRect(bool bUseNew) const
{
	if( (_uButtonState & UISTATE_CAPTURED) != 0 && bUseNew) {
		if( _iSepWidth >= 0 ) return Rect(_rcNewPos.right - _iSepWidth, _rcNewPos.top, _rcNewPos.right, _rcNewPos.bottom);
		else return Rect(_rcNewPos.left, _rcNewPos.top, _rcNewPos.left - _iSepWidth, _rcNewPos.bottom);
	}
	else {
		if( _iSepWidth >= 0 ) return Rect(_rcItem.right - _iSepWidth, _rcItem.top, _rcItem.right, _rcItem.bottom);
		else return Rect(_rcItem.left, _rcItem.top, _rcItem.left - _iSepWidth, _rcItem.bottom);
	}
}

}

#endif // ZGUI_USE_CONTAINER