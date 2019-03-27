#include "zgui.h"

#ifdef ZGUI_USE_TEXT

namespace zgui
{
	CTextUI::CTextUI() : m_nLinks(0), m_nHoverLink(-1)
	{
		m_uTextStyle = DT_WORDBREAK;
		m_rcTextPadding.left = 2;
		m_rcTextPadding.right = 2;
		::ZeroMemory(m_rcLinks, sizeof(m_rcLinks));
	}

	CTextUI::~CTextUI()
	{
	}

	LPCTSTR CTextUI::GetClass() const
	{
		return _T("TextUI");
	}

	LPVOID CTextUI::GetInterface(LPCTSTR pstrName)
	{
		if( lstrcmp(pstrName, _T("Text")) == 0 ) return static_cast<CTextUI*>(this);
		return CLabelUI::GetInterface(pstrName);
	}

	UINT CTextUI::GetControlFlags() const
	{
		if( IsEnabled() && m_nLinks > 0 ) return UIFLAG_SETCURSOR;
		else return 0;
	}

	String* CTextUI::GetLinkContent(int iIndex)
	{
		if( iIndex >= 0 && iIndex < m_nLinks ) return &m_sLinks[iIndex];
		return NULL;
	}

	void CTextUI::DoEvent(TEventUI& event)
	{
		if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
			if( _pParent != NULL ) _pParent->DoEvent(event);
			else CLabelUI::DoEvent(event);
			return;
		}

		if( event.Type == UIEVENT_SETCURSOR ) {
			for( int i = 0; i < m_nLinks; i++ ) {
				if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
					::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
					return;
				}
			}
		}
		if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK && IsEnabled() ) {
			for( int i = 0; i < m_nLinks; i++ ) {
				if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
					Invalidate();
					return;
				}
			}
		}
		if( event.Type == UIEVENT_BUTTONUP && IsEnabled() ) {
			for( int i = 0; i < m_nLinks; i++ ) {
				if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
					_pManager->SendNotify(this, "link", i);
					return;
				}
			}
		}
		if( event.Type == UIEVENT_CONTEXTMENU )
		{
			return;
		}
		// When you move over a link
		if( m_nLinks > 0 && event.Type == UIEVENT_MOUSEMOVE && IsEnabled() ) {
			int nHoverLink = -1;
			for( int i = 0; i < m_nLinks; i++ ) {
				if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
					nHoverLink = i;
					break;
				}
			}

			if(m_nHoverLink != nHoverLink) {
				m_nHoverLink = nHoverLink;
				Invalidate();
				return;
			}      
		}
		if( event.Type == UIEVENT_MOUSELEAVE ) {
			if( m_nLinks > 0 && IsEnabled() ) {
				if(m_nHoverLink != -1) {
					m_nHoverLink = -1;
					Invalidate();
					return;
				}
			}
		}

		CLabelUI::DoEvent(event);
	}

	SIZE CTextUI::EstimateSize(SIZE szAvailable)
	{
		RECT rcText = { 0, 0, MAX(szAvailable.cx, _cxyFixed.cx), 9999 };
		rcText.left += m_rcTextPadding.left;
		rcText.right -= m_rcTextPadding.right;
		if( m_bShowHtml ) {   
			int nLinks = 0;
			CRenderEngine::DrawHtmlText(_pManager->GetPaintDC(), _pManager, rcText, _text, m_dwTextColor, NULL, NULL, nLinks, DT_CALCRECT | m_uTextStyle);
		}
		else {
			CRenderEngine::DrawText(_pManager->GetPaintDC(), _pManager, rcText, _text, m_dwTextColor, m_iFont, DT_CALCRECT | m_uTextStyle);
		}
		SIZE cXY = {rcText.right - rcText.left + m_rcTextPadding.left + m_rcTextPadding.right, rcText.bottom - rcText.top + m_rcTextPadding.top + m_rcTextPadding.bottom};

        if (_cxyFixed.cy != 0) {
            cXY.cy = _cxyFixed.cy;
        }

		return cXY;
	}

	void CTextUI::PaintText(HDC hDC)
	{
		if (_text.isEmpty()) {
			m_nLinks = 0;
			return;
		}

        if (m_dwTextColor == 0) {
            m_dwTextColor = _pManager->GetDefaultFontColor();
        }
        if (m_dwDisabledTextColor == 0) {
            m_dwDisabledTextColor = _pManager->GetDefaultDisabledColor();
        }

        if (_text.isEmpty()) {
            return;
        }

		m_nLinks = lengthof(m_rcLinks);
		RECT rc = _rcItem;
		rc.left += m_rcTextPadding.left;
		rc.right -= m_rcTextPadding.right;
		rc.top += m_rcTextPadding.top;
		rc.bottom -= m_rcTextPadding.bottom;
		if (IsEnabled()) {
            if (m_bShowHtml) {
				CRenderEngine::DrawHtmlText(hDC, _pManager, rc, _text, m_dwTextColor, m_rcLinks, m_sLinks, m_nLinks, m_uTextStyle);
            }
            else {
				CRenderEngine::DrawText(hDC, _pManager, rc, _text, m_dwTextColor, m_iFont, m_uTextStyle);
            }
		}
		else {
            if (m_bShowHtml) {
				CRenderEngine::DrawHtmlText(hDC, _pManager, rc, _text, m_dwDisabledTextColor, m_rcLinks, m_sLinks, m_nLinks, m_uTextStyle);
            }
            else {
				CRenderEngine::DrawText(hDC, _pManager, rc, _text, m_dwDisabledTextColor, m_iFont, m_uTextStyle);
            }
		}
	}
}

#endif // ZGUI_USE_TEXT