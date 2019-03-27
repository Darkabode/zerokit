#ifndef __ZGUI_CHILDLAYOUT_H_
#define __ZGUI_CHILDLAYOUT_H_

#ifdef ZGUI_USE_CHILDLAYOUT

namespace zgui
{
	class CChildLayoutUI : public CContainerUI
	{
	public:
		CChildLayoutUI();

		void Init();
		void SetAttribute(const String& pstrName, const String& pstrValue);
		void SetChildLayoutXML(CDuiString pXML);
		zgui::CDuiString GetChildLayoutXML();
		virtual LPVOID GetInterface(LPCTSTR pstrName);
		virtual LPCTSTR GetClass() const;

	private:
		zgui::CDuiString m_pstrXMLFile;
	};
} // namespace zgui

#endif // ZGUI_USE_CHILDLAYOUT

#endif // __ZGUI_CHILDLAYOUT_H_