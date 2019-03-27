#ifndef __ZGUI_TABLAYOUT_H_
#define __ZGUI_TABLAYOUT_H_

#ifdef ZGUI_USE_TABLAYOUT

namespace zgui
{
	class CTabLayoutUI : public CContainerUI
	{
	public:
		CTabLayoutUI();

		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);

		bool Add(CControlUI* pControl);
		bool AddAt(CControlUI* pControl, int iIndex);
		bool Remove(CControlUI* pControl);
		void RemoveAll();
		int GetCurSel() const;
		bool SelectItem(int iIndex);

		void SetPos(RECT rc);

		void SetAttribute(const String& pstrName, const String& pstrValue);

	protected:
		int m_iCurSel;
	};
}

#endif // ZGUI_USE_TABLAYOUT

#endif // __ZGUI_TABLAYOUT_H_