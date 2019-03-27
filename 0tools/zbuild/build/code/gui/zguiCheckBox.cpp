#include "zgui.h"

#ifdef ZGUI_USE_OPTION

#ifdef ZGUI_USE_CHECKBOX

namespace zgui
{
	LPCTSTR CCheckBoxUI::GetClass() const
	{
		return _T("CheckBoxUI");
	}

	void CCheckBoxUI::SetCheck(bool bCheck)
	{
		Selected(bCheck);
	}

	bool  CCheckBoxUI::GetCheck() const
	{
		return IsSelected();
	}
}

#endif // ZGUI_USE_CHECKBOX

#endif // ZGUI_USE_OPTION