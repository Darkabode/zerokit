#ifndef __ZGUI_OPTION_H_
#define __ZGUI_OPTION_H_

#ifdef ZGUI_USE_OPTION

namespace zgui
{
	class COptionUI : public CButtonUI
	{
	public:
		COptionUI();
		~COptionUI();

		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);

		void SetManager(CPaintManagerUI* pManager, CControlUI* pParent, bool bInit = true);

		bool Activate();
		void SetEnabled(bool bEnable = true);

		const String& GetSelectedImage();
		void SetSelectedImage(const String& pStrImage);

		void SetSelectedTextColor(DWORD dwTextColor);
		DWORD GetSelectedTextColor();

		const String& GetForeImage();
		void SetForeImage(const String& pStrImage);

		const String& GetGroup() const;
        void SetGroup(const String& pStrGroupName = String::empty);
		bool IsSelected() const;
		virtual void Selected(bool bSelected);

		SIZE EstimateSize(SIZE szAvailable);
		void SetAttribute(const String& pstrName, const String& pstrValue);

		void PaintStatusImage(HDC hDC);
		void PaintText(HDC hDC);

	protected:
		bool m_bSelected;
		String _groupName;

		DWORD m_dwSelectedTextColor;

		String _selectedImageName;
		String _foreImageName;
	};

} // namespace zgui

#endif // ZGUI_USE_OPTION

#endif // __ZGUI_OPTION_H_