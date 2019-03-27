#ifndef __ZGUI_BUTTON_H_
#define __ZGUI_BUTTON_H_

#if defined(ZGUI_USE_LABEL) && defined(ZGUI_USE_BUTTON)

namespace zgui {

class Button : public Label
{
public:
	Button();
	~Button();

	const String& getClass() const;
	LPVOID getInterface(const String& name);
	UINT GetControlFlags() const;

	void SetManager(PaintManager* pManager, Control* pParent, bool bInit = true);

	bool Activate();
	void SetEnabled(bool bEnable = true);
	void DoEvent(TEventUI& event);

	const String& GetNormalImage();
	void SetNormalImage(const String& pStrImage);
	const String& GetHotImage();
	void SetHotImage(const String& pStrImage);
	const String& GetPushedImage();
	void SetPushedImage(const String& pStrImage);
	const String& GetFocusedImage();
	void SetFocusedImage(const String& pStrImage);
	const String& GetDisabledImage();
	void SetDisabledImage(const String& pStrImage);
    const String& GetForeImage();
    void SetForeImage(const String& pStrImage);
    const String& GetHotForeImage();
    void SetHotForeImage(const String& pStrImage);
	const String& GetPushedForeImage();
	void SetPushedForeImage(const String& pStrImage);

    void SetHotBkColor(DWORD dwColor);
    DWORD GetHotBkColor() const;		
	void SetPushedBkColor(DWORD dwColor);
	DWORD GetPushedBkColor() const;
	void setDisabledBkColor(DWORD dwColor);
	DWORD getDisabledBkColor() const;
	void SetHotTextColor(DWORD dwColor);
	DWORD GetHotTextColor() const;
	void SetPushedTextColor(DWORD dwColor);
	DWORD GetPushedTextColor() const;
	void SetFocusedTextColor(DWORD dwColor);
	DWORD GetFocusedTextColor() const;
	const String& GetGroup() const;
	void SetGroup(const String& pStrGroupName = String::empty);
	SIZE EstimateSize(SIZE szAvailable);
	void setAttribute(const String& pstrName, const String& pstrValue);

	bool isSelected() const;
	virtual void setSelected(bool selected);
	virtual void setToggledOnClicking(bool toggled);

	void PaintText(HDC hDC);
	void PaintStatusImage(HDC hDC);

protected:
	UINT _buttonState;

    DWORD _dwHotBkColor;
	DWORD _dwPushedBkColor;
	DWORD _dwDisabledBkColor;
	DWORD _dwHotTextColor;
	DWORD _dwPushedTextColor;
	DWORD _dwFocusedTextColor;

	String _normalImageName;
	String _hotImageName;
    String _sHotForeImage;
	String _pushedImageName;
    String _sPushedForeImage;
	String _focusedImageName;
	String _disabledImageName;

	bool _selected;
	bool _toggledOnClicking;
	bool _textSized;
	String _groupName;

private:
	static const String CLASS_NAME;
};

}	// namespace zgui

#endif // ZGUI_USE_BUTTON && ZGUI_USE_LABEL

#endif // __ZGUI_BUTTON_H_