#ifndef __ZGUI_PROGRESS_H_
#define __ZGUI_PROGRESS_H_

#ifdef ZGUI_USE_PROGRESS

namespace zgui
{
	class CProgressUI : public CLabelUI
	{
	public:
		CProgressUI();

		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);

		bool IsHorizontal();
		void SetHorizontal(bool bHorizontal = true);
		bool IsStretchForeImage();
		void SetStretchForeImage(bool bStretchForeImage = true);
		int GetMinValue() const;
		void SetMinValue(int nMin);
		int GetMaxValue() const;
		void SetMaxValue(int nMax);
		int GetValue() const;
		void SetValue(int nValue);
		LPCTSTR GetForeImage() const;
		void SetForeImage(LPCTSTR pStrImage);

		void SetAttribute(const String& pstrName, const String& pstrValue);
		void PaintStatusImage(HDC hDC);

	protected:
		bool m_bHorizontal;
		bool m_bStretchForeImage;
		int m_nMax;
		int m_nMin;
		int m_nValue;

		CDuiString m_sForeImage;
		CDuiString m_sForeImageModify;
	};

} // namespace zgui

#endif // ZGUI_USE_PROGRESS

#endif // __ZGUI_PROGRESS_H_
