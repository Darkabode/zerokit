#include "zgui.h"

#ifdef ZGUI_USE_CONTAINER

namespace zgui {

const String VerticalLayout::CLASS_NAME = ZGUI_VERTICALLAYOUT;

VerticalLayout::VerticalLayout() :
m_iSepHeight(0),
m_uButtonState(0),
m_bImmMode(false),
_childSize(false)
{
	ptLastMouse.x = ptLastMouse.y = 0;
	__stosb((uint8_t*)&m_rcNewPos, 0, sizeof(m_rcNewPos));
}

const String& VerticalLayout::getClass() const
{
	return CLASS_NAME;
}

LPVOID VerticalLayout::getInterface(const String& name)
{
	if (name == ZGUI_VERTICALLAYOUT) {
        return static_cast<VerticalLayout*>(this);
    }
	return Container::getInterface(name);
}

UINT VerticalLayout::GetControlFlags() const
{
	if (IsEnabled() && m_iSepHeight != 0) {
		return UIFLAG_SETCURSOR;
	}
	else {
		return 0;
	}
}

void VerticalLayout::SetPos(RECT rc)
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

    if (_pVerticalScrollBar != 0 && _pVerticalScrollBar->IsVisible()) {
        rc.right -= _pVerticalScrollBar->GetFixedWidth();
    }
    if (_pHorizontalScrollBar != 0 && _pHorizontalScrollBar->IsVisible()) {
        rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();
    }

	// Determine the minimum size
	SIZE szAvailable = {rc.right - rc.left, rc.bottom - rc.top};
    if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
		szAvailable.cx += _pHorizontalScrollBar->GetScrollRange();
    }

	int nAdjustables = 0;
	int cyFixed = 0;
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
		nEstimateNum++;
	}
	cyFixed += (nEstimateNum - 1) * m_iChildPadding;

    int cyExpand = 0;
	int cyNeeded = 0;	
    if (nAdjustables > 0) {
        cyExpand = MAX(0, (szAvailable.cy - cyFixed) / nAdjustables);
    }
	// Position the elements
	SIZE szRemaining = szAvailable;
	int iPosY = rc.top;
	if (_pVerticalScrollBar != 0 && _pVerticalScrollBar->IsVisible()) {
		iPosY -= _pVerticalScrollBar->GetScrollPos();
	}
	int iPosX = rc.left;
	if( _pHorizontalScrollBar != 0 && _pHorizontalScrollBar->IsVisible() ) {
		iPosX -= _pHorizontalScrollBar->GetScrollPos();
	}
	int iAdjustable = 0;
	int cyFixedRemaining = cyFixed;
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
		szRemaining.cy -= rcPadding.top;
		SIZE sz = pControl->EstimateSize(szRemaining);
		if( sz.cy == 0 ) {
			iAdjustable++;
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

		sz.cx = pControl->GetFixedWidth();
        if (sz.cx == 0) {
            sz.cx = szAvailable.cx - rcPadding.left - rcPadding.right;
        }
        if (sz.cx < 0) {
            sz.cx = 0;
        }
        if (sz.cx < pControl->GetMinWidth()) {
            sz.cx = pControl->GetMinWidth();
        }
        if (sz.cx > pControl->GetMaxWidth()) {
            sz.cx = pControl->GetMaxWidth();
        }

		RECT rcCtrl = {iPosX + rcPadding.left, iPosY + rcPadding.top, iPosX + rcPadding.left + sz.cx, iPosY + sz.cy + rcPadding.top + rcPadding.bottom};
		pControl->SetPos(rcCtrl);
		iPosY += sz.cy + m_iChildPadding + rcPadding.top + rcPadding.bottom;
		cyNeeded += sz.cy + rcPadding.top + rcPadding.bottom;
		szRemaining.cy -= sz.cy + m_iChildPadding + rcPadding.bottom;
	}
	cyNeeded += (nEstimateNum - 1) * m_iChildPadding;

	// Process the scrollbar
	if (_bNoInsetVScroll) {
		rc.right += _rcInset.right;
	}
	processScrollBar(rc, 0, cyNeeded);
}

void VerticalLayout::DoPostPaint(HDC hDC, const RECT& /*rcPaint*/)
{
	if( (m_uButtonState & UISTATE_CAPTURED) != 0 && !m_bImmMode ) {
		RECT rcSeparator = GetThumbRect(true);
		RenderEngine::DrawColor(hDC, rcSeparator, 0xAA000000);
	}
}

void VerticalLayout::SetSepHeight(int iHeight)
{
	m_iSepHeight = iHeight;
}

int VerticalLayout::GetSepHeight() const
{
	return m_iSepHeight;
}

void VerticalLayout::SetSepImmMode(bool bImmediately)
{
	if( m_bImmMode == bImmediately ) return;
	if( (m_uButtonState & UISTATE_CAPTURED) != 0 && !m_bImmMode && _pManager != NULL ) {
		_pManager->RemovePostPaint(this);
	}

	m_bImmMode = bImmediately;
}

void VerticalLayout::SetChildSize(bool childSize)
{
	_childSize = childSize;
}

bool VerticalLayout::IsSepImmMode() const
{
	return m_bImmMode;
}

void VerticalLayout::setAttribute(const String& name, const String& value)
{
    if (name == "sepheight") {
        SetSepHeight(value.getIntValue());
    }
    else if (name == "sepimm") {
        SetSepImmMode(value == "true");
    }
	else if (name == "childsize") {
		SetChildSize(value == "true");
	}
    else {
        Container::setAttribute(name, value);
    }
}

void VerticalLayout::DoEvent(TEventUI& event)
{
	if (m_iSepHeight != 0) {
		if (event.Type == UIEVENT_BUTTONDOWN && IsEnabled()) {
			RECT rcSeparator = GetThumbRect(false);
			if (fn_PtInRect(&rcSeparator, event.ptMouse)) {
				m_uButtonState |= UISTATE_CAPTURED;
				ptLastMouse = event.ptMouse;
				m_rcNewPos = _rcItem;
				if (!m_bImmMode && _pManager) {
					_pManager->AddPostPaint(this);
				}
				return;
			}
		}
		if (event.Type == UIEVENT_BUTTONUP) {
			if ((m_uButtonState & UISTATE_CAPTURED) != 0) {
				m_uButtonState &= ~UISTATE_CAPTURED;
				_rcItem = m_rcNewPos;
				if (!m_bImmMode && _pManager != 0) {
					_pManager->RemovePostPaint(this);
				}
				NeedParentUpdate();
				return;
			}
		}
		if (event.Type == UIEVENT_MOUSEMOVE) {
			if ((m_uButtonState & UISTATE_CAPTURED) != 0) {
				LONG cy = event.ptMouse.y - ptLastMouse.y;
				ptLastMouse = event.ptMouse;
				RECT rc = m_rcNewPos;
				if (m_iSepHeight >= 0) {
					if( cy > 0 && event.ptMouse.y < m_rcNewPos.bottom + m_iSepHeight ) return;
					if( cy < 0 && event.ptMouse.y > m_rcNewPos.bottom ) return;
					rc.bottom += cy;
					if( rc.bottom - rc.top <= GetMinHeight() ) {
						if( m_rcNewPos.bottom - m_rcNewPos.top <= GetMinHeight() ) return;
						rc.bottom = rc.top + GetMinHeight();
					}
					if( rc.bottom - rc.top >= GetMaxHeight() ) {
						if( m_rcNewPos.bottom - m_rcNewPos.top >= GetMaxHeight() ) return;
						rc.bottom = rc.top + GetMaxHeight();
					}
				}
				else {
					if( cy > 0 && event.ptMouse.y < m_rcNewPos.top ) return;
					if( cy < 0 && event.ptMouse.y > m_rcNewPos.top + m_iSepHeight ) return;
					rc.top += cy;
					if( rc.bottom - rc.top <= GetMinHeight() ) {
						if( m_rcNewPos.bottom - m_rcNewPos.top <= GetMinHeight() ) return;
						rc.top = rc.bottom - GetMinHeight();
					}
					if( rc.bottom - rc.top >= GetMaxHeight() ) {
						if( m_rcNewPos.bottom - m_rcNewPos.top >= GetMaxHeight() ) return;
						rc.top = rc.bottom - GetMaxHeight();
					}
				}

				Rect rcInvalidate = GetThumbRect(true);
				m_rcNewPos = rc;
				_cxyFixed.cy = m_rcNewPos.bottom - m_rcNewPos.top;

				if( m_bImmMode ) {
					_rcItem = m_rcNewPos;
					NeedParentUpdate();
				}
				else {
					rcInvalidate.joinWith(GetThumbRect(true));
					rcInvalidate.joinWith(GetThumbRect(false));
					if( _pManager ) _pManager->Invalidate(rcInvalidate);
				}
				return;
			}
		}
		if (event.Type == UIEVENT_SETCURSOR) {
			RECT rcSeparator = GetThumbRect(false);
			if (IsEnabled() && fn_PtInRect(&rcSeparator, event.ptMouse)) {
				fn_SetCursor(fn_LoadCursorW(NULL, MAKEINTRESOURCE(IDC_SIZENS)));
				return;
			}
		}
	}
	Container::DoEvent(event);
}

SIZE VerticalLayout::EstimateSize(SIZE szAvailable)
{
	if (!_childSize) {
		return Control::EstimateSize(szAvailable);
	}
	
	SIZE sz, szRemaining = szAvailable;
	__stosb((uint8_t*)&sz, 0, sizeof(sz));
	sz.cx = szAvailable.cx;
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
		szRemaining.cy -= ctrlSZ.cy;
		sz.cy += ctrlSZ.cy;
	}

	sz.cy += _rcInset.top + _rcInset.bottom;

	return sz;
}

RECT VerticalLayout::GetThumbRect(bool bUseNew) const
{
	if( (m_uButtonState & UISTATE_CAPTURED) != 0 && bUseNew) {
		if( m_iSepHeight >= 0 ) 
			return Rect(m_rcNewPos.left, MAX(m_rcNewPos.bottom - m_iSepHeight, m_rcNewPos.top), 
			m_rcNewPos.right, m_rcNewPos.bottom);
		else 
			return Rect(m_rcNewPos.left, m_rcNewPos.top, m_rcNewPos.right, 
			MIN(m_rcNewPos.top - m_iSepHeight, m_rcNewPos.bottom));
	}
	else {
		if( m_iSepHeight >= 0 ) 
			return Rect(_rcItem.left, MAX(_rcItem.bottom - m_iSepHeight, _rcItem.top), _rcItem.right, 
			_rcItem.bottom);
		else 
			return Rect(_rcItem.left, _rcItem.top, _rcItem.right, 
			MIN(_rcItem.top - m_iSepHeight, _rcItem.bottom));

	}
}
}

#endif // ZGUI_USE_CONTAINER