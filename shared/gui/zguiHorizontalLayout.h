#ifndef __UIHORIZONTALLAYOUT_H__
#define __UIHORIZONTALLAYOUT_H__

#ifdef ZGUI_USE_CONTAINER

namespace zgui
{

class HorizontalLayout : public Container
{
public:
	HorizontalLayout();

	const String& getClass() const;
	LPVOID getInterface(const String& name);
	UINT GetControlFlags() const;

	void SetSepWidth(int iWidth);
	int GetSepWidth() const;
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
	int _iSepWidth;
	UINT _uButtonState;
	POINT _ptLastMouse;
	RECT _rcNewPos;
	bool _bImmMode;
	bool _childSize;

private:
	static const String CLASS_NAME;
};

}

#endif // ZGUI_USE_CONTAINER

#endif // __UIHORIZONTALLAYOUT_H__
