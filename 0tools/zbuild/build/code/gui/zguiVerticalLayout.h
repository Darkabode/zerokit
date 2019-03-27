#ifndef __UIVERTICALLAYOUT_H__
#define __UIVERTICALLAYOUT_H__

#ifdef ZGUI_USE_CONTAINER

namespace zgui
{
	class CVerticalLayoutUI : public CContainerUI
	{
	public:
		CVerticalLayoutUI();

		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);
		UINT GetControlFlags() const;

		void SetSepHeight(int iHeight);
		int GetSepHeight() const;
		void SetSepImmMode(bool bImmediately);
		bool IsSepImmMode() const;
		void SetAttribute(const String& pstrName, const String& pstrValue);
		void DoEvent(TEventUI& event);

		void SetPos(RECT rc);
		void DoPostPaint(HDC hDC, const RECT& rcPaint);

		RECT GetThumbRect(bool bUseNew = false) const;

	protected:
		int m_iSepHeight;
		UINT m_uButtonState;
		POINT ptLastMouse;
		RECT m_rcNewPos;
		bool m_bImmMode;
	};
}

#endif // ZGUI_USE_CONTAINER

#endif // __UIVERTICALLAYOUT_H__
