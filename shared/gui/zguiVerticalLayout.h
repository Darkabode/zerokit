#ifndef __ZGUI_VERTICALLAYOUT_H_
#define __ZGUI_VERTICALLAYOUT_H_

#ifdef ZGUI_USE_CONTAINER

namespace zgui {

class VerticalLayout : public Container
{
public:
	VerticalLayout();

	const String& getClass() const;
	LPVOID getInterface(const String& name);
	UINT GetControlFlags() const;

	void SetSepHeight(int iHeight);
	int GetSepHeight() const;
	void SetSepImmMode(bool bImmediately);
	void SetChildSize(bool childSize);
	bool IsSepImmMode() const;
	void setAttribute(const String& pstrName, const String& pstrValue);
	void DoEvent(TEventUI& event);

	SIZE EstimateSize(SIZE szAvailable);

	void SetPos(RECT rc);
	void DoPostPaint(HDC hDC, const RECT& rcPaint);

	RECT GetThumbRect(bool bUseNew = false) const;

protected:
	int m_iSepHeight;
	UINT m_uButtonState;
	POINT ptLastMouse;
	RECT m_rcNewPos;
	bool m_bImmMode;
	bool _childSize;

private:
	static const String CLASS_NAME;
};

}

#endif // ZGUI_USE_CONTAINER

#endif // __ZGUI_VERTICALLAYOUT_H_
