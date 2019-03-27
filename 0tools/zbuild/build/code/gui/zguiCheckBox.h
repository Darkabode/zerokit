#ifndef __ZGUI_CHECKBOX_H_
#define __ZGUI_CHECKBOX_H_

#ifdef ZGUI_USE_OPTION

#ifdef ZGUI_USE_CHECKBOX

namespace zgui
{
	/// 最普通的单选按钮控件，只有是、否两种结果
	/// 派生于COptionUI，只是每组只有一个按钮而已，组名为空，配置文件默认属性举例：
	/// <CheckBox name="CheckBox" value="height='20' align='left' textpadding='24,0,0,0' normalimage='file='sys_check_btn.png' source='0,0,20,20' dest='0,0,20,20'' selectedimage='file='sys_check_btn.png' source='20,0,40,20' dest='0,0,20,20'' disabledimage='file='sys_check_btn.png' source='40,0,60,20' dest='0,0,20,20''"/>

	class CCheckBoxUI : public COptionUI
	{
	public:
		LPCTSTR GetClass() const;

		void SetCheck(bool bCheck);
		bool GetCheck() const;
	};
}

#endif // ZGUI_USE_CHECKBOX

#endif // ZGUI_USE_OPTION

#endif // __ZGUI_CHECKBOX_H_
