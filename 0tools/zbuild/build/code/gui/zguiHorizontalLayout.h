#ifndef __UIHORIZONTALLAYOUT_H__
#define __UIHORIZONTALLAYOUT_H__

#ifdef ZGUI_USE_CONTAINER

namespace zgui
{

class CHorizontalLayoutUI : public CContainerUI
{
public:
	CHorizontalLayoutUI();

	LPCTSTR GetClass() const;
	LPVOID GetInterface(LPCTSTR pstrName);
	UINT GetControlFlags() const;

	void SetSepWidth(int iWidth);
	int GetSepWidth() const;
	void SetSepImmMode(bool bImmediately);
	bool IsSepImmMode() const;
	void SetAttribute(const String& pstrName, const String& pstrValue);
	void DoEvent(TEventUI& event);

	void SetPos(RECT rc);
	void DoPostPaint(HDC hDC, const RECT& rcPaint);

	RECT GetThumbRect(bool bUseNew = false) const;

protected:
	int _iSepWidth;
	UINT _uButtonState;
	POINT _ptLastMouse;
	RECT _rcNewPos;
	bool _bImmMode;
};

}

#endif // ZGUI_USE_CONTAINER

#endif // __UIHORIZONTALLAYOUT_H__
