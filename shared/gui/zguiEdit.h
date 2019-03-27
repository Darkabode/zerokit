#ifndef __ZGUI_EDIT_H_
#define __ZGUI_EDIT_H_

#ifdef ZGUI_USE_LABEL

#ifdef ZGUI_USE_EDIT

namespace zgui {

class CEditWnd;

class Edit : public Label
{
	friend class CEditWnd;
public:
	Edit();

	const String& getClass() const;
	LPVOID getInterface(const String& name);
	UINT GetControlFlags() const;

	void SetEnabled(bool bEnable = true);
	void setText(const String& pstrText);
	void SetMaxChar(UINT uMax);
	UINT GetMaxChar();
	void SetReadOnly(bool bReadOnly);
	bool IsReadOnly() const;
	void SetPasswordMode(bool bPasswordMode);
	bool IsPasswordMode() const;
	void SetPasswordChar(TCHAR cPasswordChar);
	TCHAR GetPasswordChar() const;
	void SetNumberOnly(bool bNumberOnly);
	bool IsNumberOnly() const;
	int GetWindowStyls() const;

	const String& GetNormalImage();
	void SetNormalImage(const String& pStrImage);
	const String& GetHotImage();
	void SetHotImage(const String& pStrImage);
	const String& GetFocusedImage();
	void SetFocusedImage(const String& pStrImage);
	const String& GetDisabledImage();
	void SetDisabledImage(const String& pStrImage);
	void SetNativeEditBkColor(DWORD dwBkColor);
	DWORD GetNativeEditBkColor() const;

	void SetSel(long nStartChar, long nEndChar);
	void SetSelAll();
	void SetReplaceSel(LPCTSTR lpszReplace);

	void SetPos(RECT rc);
	void SetVisible(bool bVisible = true);
	void SetInternVisible(bool bVisible = true);
	SIZE EstimateSize(SIZE szAvailable);
	void DoEvent(TEventUI& event);
	void setAttribute(const String& pstrName, const String& pstrValue);

	void PaintStatusImage(HDC hDC);
	void PaintText(HDC hDC);

protected:
	CEditWnd* m_pWindow;

	UINT m_uMaxChar;
	bool m_bReadOnly;
	bool m_bPasswordMode;
	wchar_t m_cPasswordChar;
	UINT m_uButtonState;
	String _sNormalImage;
	String m_sHotImage;
	String m_sFocusedImage;
	String m_sDisabledImage;
	DWORD m_dwEditbkColor;
	int m_iWindowStyls;

private:
    static const String CLASS_NAME;
};

}

#endif // ZGUI_USE_EDIT

#endif // ZGUI_USE_LABEL

#endif // __ZGUI_EDIT_H_