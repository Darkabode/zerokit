#include "zgui.h"

#ifdef ZGUI_USE_OPTION

namespace zgui
{
	COptionUI::COptionUI() : m_bSelected(false), m_dwSelectedTextColor(0)
	{
	}

	COptionUI::~COptionUI()
	{
        if (!_groupName.isEmpty() && _pManager) {
            _pManager->RemoveOptionGroup(_groupName, this);
        }
	}

	LPCTSTR COptionUI::GetClass() const
	{
		return _T("OptionUI");
	}

	LPVOID COptionUI::GetInterface(LPCTSTR pstrName)
	{
        if (lstrcmp(pstrName, DUI_CTR_OPTION) == 0) {
            return static_cast<COptionUI*>(this);
        }
		return CButtonUI::GetInterface(pstrName);
	}

	void COptionUI::SetManager(CPaintManagerUI* pManager, CControlUI* pParent, bool bInit)
	{
		CControlUI::SetManager(pManager, pParent, bInit);
		if (bInit && !_groupName.isEmpty()) {
            if (_pManager) {
                _pManager->AddOptionGroup(_groupName, this);
            }
		}
	}

	const String& COptionUI::GetGroup() const
	{
		return _groupName;
	}

	void COptionUI::SetGroup(const String& pStrGroupName)
	{
		if (pStrGroupName.isEmpty()) {
            if (_groupName.isEmpty()) {
                return;
            }
            _groupName = String::empty;
		}
		else {
            if (_groupName == pStrGroupName) {
                return;
            }
            if (!_groupName.isEmpty() && _pManager) {
                _pManager->RemoveOptionGroup(_groupName, this);
            }
			_groupName = pStrGroupName;
		}

		if (!_groupName.isEmpty()) {
            if (_pManager) {
                _pManager->AddOptionGroup(_groupName, this);
            }
		}
		else {
            if (_pManager) {
                _pManager->RemoveOptionGroup(_groupName, this);
            }
		}

		Selected(m_bSelected);
	}

	bool COptionUI::IsSelected() const
	{
		return m_bSelected;
	}

	void COptionUI::Selected(bool bSelected)
	{
		if( m_bSelected == bSelected ) return;
		m_bSelected = bSelected;
		if( m_bSelected ) m_uButtonState |= UISTATE_SELECTED;
		else m_uButtonState &= ~UISTATE_SELECTED;

		if (_pManager != NULL) {
			if (!_groupName.isEmpty()) {
				if (m_bSelected) {
					CStdPtrArray* aOptionGroup = _pManager->GetOptionGroup(_groupName);
					for (int i = 0; i < aOptionGroup->GetSize(); ++i) {
						COptionUI* pControl = static_cast<COptionUI*>(aOptionGroup->GetAt(i));
						if (pControl != this) {
							pControl->Selected(false);
						}
					}
					_pManager->SendNotify(this, DUI_MSGTYPE_SELECTCHANGED);
				}
			}
			else {
				_pManager->SendNotify(this, DUI_MSGTYPE_SELECTCHANGED);
			}
		}

		Invalidate();
	}

	bool COptionUI::Activate()
	{
        if (!CButtonUI::Activate()) {
            return false;
        }
        if (!_groupName.isEmpty()) {
            Selected(true);
        }
        else {
            Selected(!m_bSelected);
        }

		return true;
	}

	void COptionUI::SetEnabled(bool bEnable)
	{
		CControlUI::SetEnabled(bEnable);
		if (!IsEnabled()) {
            if (m_bSelected) {
                m_uButtonState = UISTATE_SELECTED;
            }
            else {
                m_uButtonState = 0;
            }
		}
	}

	const String& COptionUI::GetSelectedImage()
	{
		return _selectedImageName;
	}

	void COptionUI::SetSelectedImage(const String& pStrImage)
	{
		_selectedImageName = pStrImage;
		Invalidate();
	}

	void COptionUI::SetSelectedTextColor(DWORD dwTextColor)
	{
		m_dwSelectedTextColor = dwTextColor;
	}

	DWORD COptionUI::GetSelectedTextColor()
	{
        if (m_dwSelectedTextColor == 0) {
            m_dwSelectedTextColor = _pManager->GetDefaultFontColor();
        }
		return m_dwSelectedTextColor;
	}

	const String& COptionUI::GetForeImage()
	{
		return _foreImageName;
	}

	void COptionUI::SetForeImage(const String& pStrImage)
	{
		_foreImageName = pStrImage;
		Invalidate();
	}

	SIZE COptionUI::EstimateSize(SIZE szAvailable)
	{
		if( _cxyFixed.cy == 0 ) return CSize(_cxyFixed.cx, _pManager->GetFontInfo(GetFont())->tm.tmHeight + 8);
		return CControlUI::EstimateSize(szAvailable);
	}

	void COptionUI::SetAttribute(const String& pstrName, const String& pstrValue)
	{
        if (pstrName == "group") {
            SetGroup(pstrValue);
        }
        else if (pstrName == "selected") {
            Selected(pstrValue == "true");
        }
        else if (pstrName == "selectedimage") {
            SetSelectedImage(pstrValue);
        }
        else if (pstrName == "foreimage") {
            SetForeImage(pstrValue);
        }
		else if (pstrName == "selectedtextcolor") {
			SetSelectedTextColor((uint32_t)pstrValue.getHexValue32());
		}
        else {
            CButtonUI::SetAttribute(pstrName, pstrValue);
        }
	}

	void COptionUI::PaintStatusImage(HDC hDC)
	{
		m_uButtonState &= ~UISTATE_PUSHED;

		if ((m_uButtonState & UISTATE_SELECTED) != 0) {
			if (!_selectedImageName.isEmpty()) {
                if (!DrawImage(hDC, _selectedImageName)) {
                    _selectedImageName = String::empty;
                }
                else {
                    goto Label_ForeImage;
                }
			}
		}

		CButtonUI::PaintStatusImage(hDC);

Label_ForeImage:
		if (!_foreImageName.isEmpty()) {
            if (!DrawImage(hDC, _foreImageName)) {
                _foreImageName = String::empty;
            }
		}
	}

	void COptionUI::PaintText(HDC hDC)
	{
		if( (m_uButtonState & UISTATE_SELECTED) != 0 )
		{
			DWORD oldTextColor = m_dwTextColor;
			if( m_dwSelectedTextColor != 0 ) m_dwTextColor = m_dwSelectedTextColor;

			if( m_dwTextColor == 0 ) m_dwTextColor = _pManager->GetDefaultFontColor();
			if( m_dwDisabledTextColor == 0 ) m_dwDisabledTextColor = _pManager->GetDefaultDisabledColor();

            if (_text.isEmpty()) {
                return;
            }
			int nLinks = 0;
			RECT rc = _rcItem;
			rc.left += m_rcTextPadding.left;
			rc.right -= m_rcTextPadding.right;
			rc.top += m_rcTextPadding.top;
			rc.bottom -= m_rcTextPadding.bottom;

			if( m_bShowHtml )
				CRenderEngine::DrawHtmlText(hDC, _pManager, rc, _text, IsEnabled()?m_dwTextColor:m_dwDisabledTextColor, \
				NULL, NULL, nLinks, m_uTextStyle);
			else
				CRenderEngine::DrawText(hDC, _pManager, rc, _text, IsEnabled()?m_dwTextColor:m_dwDisabledTextColor, \
				m_iFont, m_uTextStyle);

			m_dwTextColor = oldTextColor;
		}
		else
			CButtonUI::PaintText(hDC);
	}
}

#endif // ZGUI_USE_OPTION