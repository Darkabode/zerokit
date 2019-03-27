#include "zgui.h"

#ifdef ZGUI_USE_CHILDLAYOUT

namespace zgui
{
	CChildLayoutUI::CChildLayoutUI()
	{

	}

	void CChildLayoutUI::Init()
	{
		if (!m_pstrXMLFile.IsEmpty())
		{
			GuiBuilder builder;
			CContainerUI* pChildWindow = static_cast<CContainerUI*>(builder.Create(m_pstrXMLFile.GetData(), (UINT)0));
			if (pChildWindow)
			{
				this->Add(pChildWindow);
			}
			else
			{
				this->RemoveAll();
			}
		}
	}

	void CChildLayoutUI::setAttribute(const String& pstrName, const String& pstrValue)
	{
        if (pstrName == "xmlfile") {
			SetChildLayoutXML(pstrValue);
        }
        else {
			CContainerUI::setAttribute(pstrName, pstrValue);
        }
	}

	void CChildLayoutUI::SetChildLayoutXML( zgui::CDuiString pXML )
	{
		m_pstrXMLFile=pXML;
	}

	zgui::CDuiString CChildLayoutUI::GetChildLayoutXML()
	{
		return m_pstrXMLFile;
	}

	LPVOID CChildLayoutUI::getInterface(const String& name)
	{
        if (name == DUI_CTR_CHILDLAYOUT) {
            return static_cast<CChildLayoutUI*>(this);
        }
		return CControlUI::getInterface(name);
	}

	const String& CChildLayoutUI::getClass() const
	{
		return "ChildLayoutUI";
	}
} // namespace zgui

#endif // ZGUI_USE_CHILDLAYOUT