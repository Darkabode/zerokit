#ifndef __ZGUI_BUTTON_H_
#define __ZGUI_BUTTON_H_

#ifdef ZGUI_USE_LABEL

#ifdef ZGUI_USE_BUTTON

namespace zgui
{
	class CButtonUI : public CLabelUI
	{
	public:
		CButtonUI();

		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);
		UINT GetControlFlags() const;

		bool Activate();
		void SetEnabled(bool bEnable = true);
		void DoEvent(TEventUI& event);

		const String& GetNormalImage();
		void SetNormalImage(const String& pStrImage);
		const String& GetHotImage();
		void SetHotImage(const String& pStrImage);
		const String& GetPushedImage();
		void SetPushedImage(const String& pStrImage);
		const String& GetFocusedImage();
		void SetFocusedImage(const String& pStrImage);
		const String& GetDisabledImage();
		void SetDisabledImage(const String& pStrImage);
        const String& GetForeImage();
        void SetForeImage(const String& pStrImage);
        const String& GetHotForeImage();
        void SetHotForeImage(const String& pStrImage);

        void SetHotBkColor(DWORD dwColor);
        DWORD GetHotBkColor() const;
		void SetHotTextColor(DWORD dwColor);
		DWORD GetHotTextColor() const;
		void SetPushedTextColor(DWORD dwColor);
		DWORD GetPushedTextColor() const;
		void SetFocusedTextColor(DWORD dwColor);
		DWORD GetFocusedTextColor() const;
		SIZE EstimateSize(SIZE szAvailable);
		void SetAttribute(const String& pstrName, const String& pstrValue);

		void PaintText(HDC hDC);
		void PaintStatusImage(HDC hDC);

	protected:
		UINT m_uButtonState;

        DWORD m_dwHotBkColor;
		DWORD m_dwHotTextColor;
		DWORD m_dwPushedTextColor;
		DWORD m_dwFocusedTextColor;

		String _normalImageName;
		String _hotImageName;
        String m_sHotForeImage;
		String _pushedImageName;
        String m_sPushedForeImage;
		String _focusedImageName;
		String _disabledImageName;
	};

}	// namespace zgui

#endif // ZGUI_USE_BUTTON

#endif // ZGUI_USE_LABEL

#endif // __ZGUI_BUTTON_H_