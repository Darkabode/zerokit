#include "zgui.h"

#ifdef ZGUI_USE_CONTAINER

namespace zgui
{

CHorizontalLayoutUI::CHorizontalLayoutUI() :
_iSepWidth(0),
_uButtonState(0),
_bImmMode(false)
{
	_ptLastMouse.x = _ptLastMouse.y = 0;
	::ZeroMemory(&_rcNewPos, sizeof(_rcNewPos));
}

LPCTSTR CHorizontalLayoutUI::GetClass() const
{
	return _T("HorizontalLayoutUI");
}

LPVOID CHorizontalLayoutUI::GetInterface(LPCTSTR pstrName)
{
    if (lstrcmp(pstrName, DUI_CTR_HORIZONTALLAYOUT) == 0) {
        return static_cast<CHorizontalLayoutUI*>(this);
    }
	return CContainerUI::GetInterface(pstrName);
}

UINT CHorizontalLayoutUI::GetControlFlags() const
{
    if (IsEnabled() && _iSepWidth != 0) {
        return UIFLAG_SETCURSOR;
    }
    
    return 0;
}

void CHorizontalLayoutUI::SetPos(RECT rc)
{
	CControlUI::SetPos(rc);
	rc = _rcItem;

	// Adjust for inset
	rc.left += _rcInset.left;
	rc.top += _rcInset.top;
	rc.right -= _rcInset.right;
	rc.bottom -= _rcInset.bottom;

	if (m_items.GetSize() == 0) {
		ProcessScrollBar(rc, 0, 0);
		return;
	}

    if (_pVerticalScrollBar && _pVerticalScrollBar->IsVisible()) {
        rc.right -= _pVerticalScrollBar->GetFixedWidth();
    }
    if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
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
	for( int it1 = 0; it1 < m_items.GetSize(); it1++ ) {
		CControlUI* pControl = static_cast<CControlUI*>(m_items[it1]);
		if( !pControl->IsVisible() ) continue;
		if( pControl->IsFloat() ) continue;
		SIZE sz = pControl->EstimateSize(szAvailable);
		if( sz.cx == 0 ) {
			nAdjustables++;
		}
		else {
			if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
			if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();
		}
		cxFixed += sz.cx +  pControl->GetPadding().left + pControl->GetPadding().right;
		nEstimateNum++;
	}
	cxFixed += (nEstimateNum - 1) * m_iChildPadding;

	int cxExpand = 0;
	if( nAdjustables > 0 ) cxExpand = MAX(0, (szAvailable.cx - cxFixed) / nAdjustables);
	// Position the elements
	SIZE szRemaining = szAvailable;
	int iPosX = rc.left;
	if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) {
		iPosX -= _pHorizontalScrollBar->GetScrollPos();
	}
	int iAdjustable = 0;
	int cxFixedRemaining = cxFixed;
	for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
		CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
		if( !pControl->IsVisible() ) continue;
		if( pControl->IsFloat() ) {
			SetFloatPos(it2);
			continue;
		}
		RECT rcPadding = pControl->GetPadding();
		szRemaining.cx -= rcPadding.left;
		SIZE sz = pControl->EstimateSize(szRemaining);
		if( sz.cx == 0 ) {
			iAdjustable++;
			sz.cx = cxExpand;
			// Distribute remaining to last element (usually round-off left-overs)
			if( iAdjustable == nAdjustables ) {
				sz.cx = MAX(0, szRemaining.cx - rcPadding.right - cxFixedRemaining);
			}
			if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
			if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();
		}
		else {
			if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
			if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();

			cxFixedRemaining -= sz.cx;
		}

		sz.cy = pControl->GetFixedHeight();
		if( sz.cy == 0 ) sz.cy = rc.bottom - rc.top - rcPadding.top - rcPadding.bottom;
		if( sz.cy < 0 ) sz.cy = 0;
		if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
		if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();

		RECT rcCtrl = { iPosX + rcPadding.left, rc.top + rcPadding.top, iPosX + sz.cx + rcPadding.left + rcPadding.right, rc.top + rcPadding.top + sz.cy};
		pControl->SetPos(rcCtrl);
		iPosX += sz.cx + m_iChildPadding + rcPadding.left + rcPadding.right;
		szRemaining.cx -= sz.cx + m_iChildPadding + rcPadding.right;
	}

	// Process the scrollbar
	ProcessScrollBar(rc, 0, 0);
}

void CHorizontalLayoutUI::DoPostPaint(HDC hDC, const RECT& /*rcPaint*/)
{
	if ((_uButtonState & UISTATE_CAPTURED) != 0 && !_bImmMode) {
		RECT rcSeparator = GetThumbRect(true);
		CRenderEngine::DrawColor(hDC, rcSeparator, 0xAA000000);
	}
}

void CHorizontalLayoutUI::SetSepWidth(int iWidth)
{
	_iSepWidth = iWidth;
}

int CHorizontalLayoutUI::GetSepWidth() const
{
	return _iSepWidth;
}

void CHorizontalLayoutUI::SetSepImmMode(bool bImmediately)
{
	if( _bImmMode == bImmediately ) return;
	if( (_uButtonState & UISTATE_CAPTURED) != 0 && !_bImmMode && _pManager != NULL ) {
		_pManager->RemovePostPaint(this);
	}

	_bImmMode = bImmediately;
}

bool CHorizontalLayoutUI::IsSepImmMode() const
{
	return _bImmMode;
}

void CHorizontalLayoutUI::SetAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "sepwidth") {
        SetSepWidth(pstrValue.getIntValue());
    }
    else if (pstrName == "sepimm") {
        SetSepImmMode(pstrValue == "true");
    }
    else {
        CContainerUI::SetAttribute(pstrName, pstrValue);
    }
}

void CHorizontalLayoutUI::DoEvent(TEventUI& event)
{
	if( _iSepWidth != 0 ) {
		if( event.Type == UIEVENT_BUTTONDOWN && IsEnabled() )
		{
			RECT rcSeparator = GetThumbRect(false);
			if( ::PtInRect(&rcSeparator, event.ptMouse) ) {
				_uButtonState |= UISTATE_CAPTURED;
				_ptLastMouse = event.ptMouse;
				_rcNewPos = _rcItem;
				if( !_bImmMode && _pManager ) _pManager->AddPostPaint(this);
				return;
			}
		}
		if( event.Type == UIEVENT_BUTTONUP )
		{
			if( (_uButtonState & UISTATE_CAPTURED) != 0 ) {
				_uButtonState &= ~UISTATE_CAPTURED;
				_rcItem = _rcNewPos;
				if( !_bImmMode && _pManager ) _pManager->RemovePostPaint(this);
				NeedParentUpdate();
				return;
			}
		}
		if( event.Type == UIEVENT_MOUSEMOVE )
		{
			if( (_uButtonState & UISTATE_CAPTURED) != 0 ) {
				LONG cx = event.ptMouse.x - _ptLastMouse.x;
				_ptLastMouse = event.ptMouse;
				RECT rc = _rcNewPos;
				if( _iSepWidth >= 0 ) {
					if( cx > 0 && event.ptMouse.x < _rcNewPos.right - _iSepWidth ) return;
					if( cx < 0 && event.ptMouse.x > _rcNewPos.right ) return;
					rc.right += cx;
					if( rc.right - rc.left <= GetMinWidth() ) {
						if( _rcNewPos.right - _rcNewPos.left <= GetMinWidth() ) return;
						rc.right = rc.left + GetMinWidth();
					}
					if( rc.right - rc.left >= GetMaxWidth() ) {
						if( _rcNewPos.right - _rcNewPos.left >= GetMaxWidth() ) return;
						rc.right = rc.left + GetMaxWidth();
					}
				}
				else {
					if( cx > 0 && event.ptMouse.x < _rcNewPos.left ) return;
					if( cx < 0 && event.ptMouse.x > _rcNewPos.left - _iSepWidth ) return;
					rc.left += cx;
					if( rc.right - rc.left <= GetMinWidth() ) {
						if( _rcNewPos.right - _rcNewPos.left <= GetMinWidth() ) return;
						rc.left = rc.right - GetMinWidth();
					}
					if( rc.right - rc.left >= GetMaxWidth() ) {
						if( _rcNewPos.right - _rcNewPos.left >= GetMaxWidth() ) return;
						rc.left = rc.right - GetMaxWidth();
					}
				}

				CDuiRect rcInvalidate = GetThumbRect(true);
				_rcNewPos = rc;
				_cxyFixed.cx = _rcNewPos.right - _rcNewPos.left;

				if( _bImmMode ) {
					_rcItem = _rcNewPos;
					NeedParentUpdate();
				}
				else {
					rcInvalidate.Join(GetThumbRect(true));
					rcInvalidate.Join(GetThumbRect(false));
					if( _pManager ) _pManager->Invalidate(rcInvalidate);
				}
				return;
			}
		}
		if( event.Type == UIEVENT_SETCURSOR )
		{
			RECT rcSeparator = GetThumbRect(false);
			if( IsEnabled() && ::PtInRect(&rcSeparator, event.ptMouse) ) {
				::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE)));
				return;
			}
		}
	}
	CContainerUI::DoEvent(event);
}

RECT CHorizontalLayoutUI::GetThumbRect(bool bUseNew) const
{
	if( (_uButtonState & UISTATE_CAPTURED) != 0 && bUseNew) {
		if( _iSepWidth >= 0 ) return CDuiRect(_rcNewPos.right - _iSepWidth, _rcNewPos.top, _rcNewPos.right, _rcNewPos.bottom);
		else return CDuiRect(_rcNewPos.left, _rcNewPos.top, _rcNewPos.left - _iSepWidth, _rcNewPos.bottom);
	}
	else {
		if( _iSepWidth >= 0 ) return CDuiRect(_rcItem.right - _iSepWidth, _rcItem.top, _rcItem.right, _rcItem.bottom);
		else return CDuiRect(_rcItem.left, _rcItem.top, _rcItem.left - _iSepWidth, _rcItem.bottom);
	}
}

}

#endif // ZGUI_USE_CONTAINER