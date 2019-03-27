#ifndef __ZGUI_SLIDER_H_
#define __ZGUI_SLIDER_H_

#ifdef ZGUI_USE_SLIDER

namespace zgui {

class CSliderUI : public CProgressUI
{
public:
	CSliderUI();

	const String& getClass() const;
	UINT GetControlFlags() const;
	LPVOID getInterface(const String& name);

	void SetEnabled(bool bEnable = true);

	int GetChangeStep();
	void SetChangeStep(int step);
	void SetThumbSize(SIZE szXY);
	RECT GetThumbRect() const;
	LPCTSTR GetThumbImage() const;
	void SetThumbImage(LPCTSTR pStrImage);
	LPCTSTR GetThumbHotImage() const;
	void SetThumbHotImage(LPCTSTR pStrImage);
	LPCTSTR GetThumbPushedImage() const;
	void SetThumbPushedImage(LPCTSTR pStrImage);

	void DoEvent(TEventUI& event);
	void setAttribute(const String& pstrName, const String& pstrValue);
	void PaintStatusImage(HDC hDC);

protected:
	SIZE m_szThumb;
	UINT m_uButtonState;
	int m_nStep;

	String m_sThumbImage;
	String m_sThumbHotImage;
	String m_sThumbPushedImage;

	String m_sImageModify;
};

}

#endif // ZGUI_USE_SLIDER

#endif // __ZGUI_SLIDER_H_