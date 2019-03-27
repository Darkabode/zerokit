#ifndef __ZGUI_GUIBUILDER_H_
#define __ZGUI_GUIBUILDER_H_

namespace zgui {

class IDialogBuilderCallback
{
public:
	virtual Control* createControl(const String& className, IDialogBuilderCallback* pCallback, PaintManager* pManager) = 0;
};


class GuiBuilder
{
public:
    GuiBuilder();

    Control* create(const String& xml, IDialogBuilderCallback* pCallback = 0, PaintManager* pManager = 0, Control* pParent = 0);
    Control* create(IDialogBuilderCallback* pCallback = 0, PaintManager* pManager = 0, Control* pParent = 0);

    void getLastErrorMessage(LPTSTR pstrMessage, SIZE_T cchMax) const;
    void getLastErrorLocation(LPTSTR pstrSource, SIZE_T cchMax) const;
private:
    Control* parse(MarkupNode* parent, Control* pParent = 0, PaintManager* pManager = 0);

    Markup _xml;
    IDialogBuilderCallback* _pCallback;
};

} // namespace zgui

#endif // __ZGUI_GUIBUILDER_H_
