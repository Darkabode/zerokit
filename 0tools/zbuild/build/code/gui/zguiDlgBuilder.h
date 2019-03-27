#ifndef __ZGUI_BUILDER_H_
#define __ZGUI_BUILDER_H_

namespace zgui {

class IDialogBuilderCallback
{
public:
    virtual CControlUI* CreateControl(LPCTSTR pstrClass) = 0;
};


class GuiBuilder
{
public:
    GuiBuilder();
    CControlUI* Create(const String& xml, LPCTSTR type = NULL, IDialogBuilderCallback* pCallback = NULL, CPaintManagerUI* pManager = NULL, CControlUI* pParent = NULL);
    CControlUI* Create(IDialogBuilderCallback* pCallback = NULL, CPaintManagerUI* pManager = NULL, CControlUI* pParent = NULL);

    CMarkup* GetMarkup();

    void GetLastErrorMessage(LPTSTR pstrMessage, SIZE_T cchMax) const;
    void GetLastErrorLocation(LPTSTR pstrSource, SIZE_T cchMax) const;
private:
    CControlUI* _Parse(CMarkupNode* parent, CControlUI* pParent = NULL, CPaintManagerUI* pManager = NULL);

    CMarkup m_xml;
    IDialogBuilderCallback* m_pCallback;
    LPCTSTR m_pstrtype;
};

} // namespace zgui

#endif // __ZGUI_BUILDER_H_
