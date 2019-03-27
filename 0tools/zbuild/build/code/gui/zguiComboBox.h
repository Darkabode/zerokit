#ifndef __ZGUI_COMBOBOX_H_
#define __ZGUI_COMBOBOX_H_

#ifdef ZGUI_USE_COMBOBOX

namespace zgui
{
	/// 扩展下拉列表框
	/// 增加arrowimage属性,一张图片平均分成5份,Normal/Hot/Pushed/Focused/Disabled(必须有source属性)
	/// <Default name="ComboBox" value="arrowimage=&quot;file='sys_combo_btn.png' source='0,0,16,16'&quot; "/>
	class CComboBoxUI : public CComboUI
	{
	public:
		CComboBoxUI();
		LPCTSTR GetClass() const;

		void SetAttribute(const String& pstrName, const String& pstrValue);

		void PaintText(HDC hDC);
		void PaintStatusImage(HDC hDC);

	protected:
		CDuiString m_sArrowImage;
		int m_nArrowWidth;
	};
}

#endif // ZGUI_USE_COMBOBOX

#endif // __ZGUI_COMBOBOX_H_
