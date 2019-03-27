#include "zgui.h"

#ifdef ZGUI_USE_SLIDER

namespace zgui
{
	CSliderUI::CSliderUI() : m_uButtonState(0), m_nStep(1)
	{
		m_uTextStyle = DT_SINGLELINE | DT_CENTER;
		m_szThumb.cx = m_szThumb.cy = 10;
	}

	LPCTSTR CSliderUI::GetClass() const
	{
		return _T("SliderUI");
	}

	UINT CSliderUI::GetControlFlags() const
	{
		if( IsEnabled() ) return UIFLAG_SETCURSOR;
		else return 0;
	}

	LPVOID CSliderUI::GetInterface(LPCTSTR pstrName)
	{
		if( lstrcmp(pstrName, _T("Slider")) == 0 ) return static_cast<CSliderUI*>(this);
		return CProgressUI::GetInterface(pstrName);
	}

	void CSliderUI::SetEnabled(bool bEnable)
	{
		CControlUI::SetEnabled(bEnable);
		if( !IsEnabled() ) {
			m_uButtonState = 0;
		}
	}

	int CSliderUI::GetChangeStep()
	{
		return m_nStep;
	}

	void CSliderUI::SetChangeStep(int step)
	{
		m_nStep = step;
	}

	void CSliderUI::SetThumbSize(SIZE szXY)
	{
		m_szThumb = szXY;
	}

	RECT CSliderUI::GetThumbRect() const
	{
		if( m_bHorizontal ) {
			int left = _rcItem.left + (_rcItem.right - _rcItem.left - m_szThumb.cx) * (m_nValue - m_nMin) / (m_nMax - m_nMin);
			int top = (_rcItem.bottom + _rcItem.top - m_szThumb.cy) / 2;
			return CDuiRect(left, top, left + m_szThumb.cx, top + m_szThumb.cy); 
		}
		else {
			int left = (_rcItem.right + _rcItem.left - m_szThumb.cx) / 2;
			int top = _rcItem.bottom - m_szThumb.cy - (_rcItem.bottom - _rcItem.top - m_szThumb.cy) * (m_nValue - m_nMin) / (m_nMax - m_nMin);
			return CDuiRect(left, top, left + m_szThumb.cx, top + m_szThumb.cy); 
		}
	}

	LPCTSTR CSliderUI::GetThumbImage() const
	{
		return m_sThumbImage;
	}

	void CSliderUI::SetThumbImage(LPCTSTR pStrImage)
	{
		m_sThumbImage = pStrImage;
		Invalidate();
	}

	LPCTSTR CSliderUI::GetThumbHotImage() const
	{
		return m_sThumbHotImage;
	}

	void CSliderUI::SetThumbHotImage(LPCTSTR pStrImage)
	{
		m_sThumbHotImage = pStrImage;
		Invalidate();
	}

	LPCTSTR CSliderUI::GetThumbPushedImage() const
	{
		return m_sThumbPushedImage;
	}

	void CSliderUI::SetThumbPushedImage(LPCTSTR pStrImage)
	{
		m_sThumbPushedImage = pStrImage;
		Invalidate();
	}

	void CSliderUI::DoEvent(TEventUI& event)
	{
		if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
			if( _pParent != NULL ) _pParent->DoEvent(event);
			else CProgressUI::DoEvent(event);
			return;
		}

		if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK )
		{
			if( IsEnabled() ) {
				RECT rcThumb = GetThumbRect();
				if( ::PtInRect(&rcThumb, event.ptMouse) ) {
					m_uButtonState |= UISTATE_CAPTURED;
				}
			}
			return;
		}
		if( event.Type == UIEVENT_BUTTONUP )
		{
			if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
				m_uButtonState &= ~UISTATE_CAPTURED;
			}
			if( m_bHorizontal ) {
				if( event.ptMouse.x >= _rcItem.right - m_szThumb.cx / 2 ) m_nValue = m_nMax;
				else if( event.ptMouse.x <= _rcItem.left + m_szThumb.cx / 2 ) m_nValue = m_nMin;
				else m_nValue = m_nMin + (m_nMax - m_nMin) * (event.ptMouse.x - _rcItem.left - m_szThumb.cx / 2 ) / (_rcItem.right - _rcItem.left - m_szThumb.cx);
			}
			else {
				if( event.ptMouse.y >= _rcItem.bottom - m_szThumb.cy / 2 ) m_nValue = m_nMin;
				else if( event.ptMouse.y <= _rcItem.top + m_szThumb.cy / 2  ) m_nValue = m_nMax;
				else m_nValue = m_nMin + (m_nMax - m_nMin) * (_rcItem.bottom - event.ptMouse.y - m_szThumb.cy / 2 ) / (_rcItem.bottom - _rcItem.top - m_szThumb.cy);
			}
			_pManager->SendNotify(this, "valuechanged");
			Invalidate();
			return;
		}
		if( event.Type == UIEVENT_CONTEXTMENU )
		{
			return;
		}
		if( event.Type == UIEVENT_SCROLLWHEEL ) 
		{
			switch( LOWORD(event.wParam) ) {
		case SB_LINEUP:
			SetValue(GetValue() + GetChangeStep());
			_pManager->SendNotify(this, "valuechanged");
			return;
		case SB_LINEDOWN:
			SetValue(GetValue() - GetChangeStep());
			_pManager->SendNotify(this, "valuechanged");
			return;
			}
		}
		if( event.Type == UIEVENT_MOUSEMOVE )
		{
			if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
				if( m_bHorizontal ) {
					if( event.ptMouse.x >= _rcItem.right - m_szThumb.cx / 2 ) m_nValue = m_nMax;
					else if( event.ptMouse.x <= _rcItem.left + m_szThumb.cx / 2 ) m_nValue = m_nMin;
					else m_nValue = m_nMin + (m_nMax - m_nMin) * (event.ptMouse.x - _rcItem.left - m_szThumb.cx / 2 ) / (_rcItem.right - _rcItem.left - m_szThumb.cx);
				}
				else {
					if( event.ptMouse.y >= _rcItem.bottom - m_szThumb.cy / 2 ) m_nValue = m_nMin;
					else if( event.ptMouse.y <= _rcItem.top + m_szThumb.cy / 2  ) m_nValue = m_nMax;
					else m_nValue = m_nMin + (m_nMax - m_nMin) * (_rcItem.bottom - event.ptMouse.y - m_szThumb.cy / 2 ) / (_rcItem.bottom - _rcItem.top - m_szThumb.cy);
				}
				Invalidate();
			}
			return;
		}
		if( event.Type == UIEVENT_SETCURSOR )
		{
			RECT rcThumb = GetThumbRect();
			if( IsEnabled() && ::PtInRect(&rcThumb, event.ptMouse) ) {
				::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
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


	void CSliderUI::SetAttribute(const String& pstrName, const String& pstrValue)
	{
        if (pstrName == "thumbimage") {
            SetThumbImage(pstrValue);
        }
        else if (pstrName == "thumbhotimage") {
            SetThumbHotImage(pstrValue);
        }
        else if (pstrName == "thumbpushedimage") {
            SetThumbPushedImage(pstrValue);
        }
		else if (pstrName == "thumbsize") {
			SIZE szXY = {0};
            if (Helper::splitString(pstrValue, ",", String::empty, szXY.cx, szXY.cy)) {
                SetThumbSize(szXY);
            }
		}
		else if (pstrName == "step") {
			SetChangeStep(pstrValue.getIntValue());
		}
		else CProgressUI::SetAttribute(pstrName, pstrValue);
	}

	void CSliderUI::PaintStatusImage(HDC hDC)
	{
		CProgressUI::PaintStatusImage(hDC);

		RECT rcThumb = GetThumbRect();
		rcThumb.left -= _rcItem.left;
		rcThumb.top -= _rcItem.top;
		rcThumb.right -= _rcItem.left;
		rcThumb.bottom -= _rcItem.top;
		if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
			if( !m_sThumbPushedImage.IsEmpty() ) {
				m_sImageModify.Empty();
				m_sImageModify.SmallFormat(_T("dest='%d,%d,%d,%d'"), rcThumb.left, rcThumb.top, rcThumb.right, rcThumb.bottom);
				if( !DrawImage(hDC, (LPCTSTR)m_sThumbPushedImage, (LPCTSTR)m_sImageModify) ) m_sThumbPushedImage.Empty();
				else return;
			}
		}
		else if( (m_uButtonState & UISTATE_HOT) != 0 ) {
			if( !m_sThumbHotImage.IsEmpty() ) {
				m_sImageModify.Empty();
				m_sImageModify.SmallFormat(_T("dest='%d,%d,%d,%d'"), rcThumb.left, rcThumb.top, rcThumb.right, rcThumb.bottom);
				if( !DrawImage(hDC, (LPCTSTR)m_sThumbHotImage, (LPCTSTR)m_sImageModify) ) m_sThumbHotImage.Empty();
				else return;
			}
		}

		if( !m_sThumbImage.IsEmpty() ) {
			m_sImageModify.Empty();
			m_sImageModify.SmallFormat(_T("dest='%d,%d,%d,%d'"), rcThumb.left, rcThumb.top, rcThumb.right, rcThumb.bottom);
			if( !DrawImage(hDC, (LPCTSTR)m_sThumbImage, (LPCTSTR)m_sImageModify) ) m_sThumbImage.Empty();
			else return;
		}
	}
}

#endif // ZGUI_USE_SLIDER