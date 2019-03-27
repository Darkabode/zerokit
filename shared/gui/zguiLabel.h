#ifndef __ZGUI_LABEL_H_
#define __ZGUI_LABEL_H_

#ifdef ZGUI_USE_LABEL

namespace zgui {

class Label : public Control
{
public:
	Label();

	const String& getClass() const;
	LPVOID getInterface(const String& name);

	void SetTextStyle(UINT uStyle);
	UINT GetTextStyle() const;
	void SetTextColor(DWORD dwTextColor);
	uint32_t GetTextColor() const;
	void SetDisabledTextColor(DWORD dwTextColor);
	DWORD GetDisabledTextColor() const;
	void SetFont(int index);
	int GetFont() const;
	RECT GetTextPadding() const;
	void SetTextPadding(RECT rc);
	bool IsShowHtml();
	void SetShowHtml(bool bShowHtml = true);

	SIZE EstimateSize(SIZE szAvailable);
	void DoEvent(TEventUI& event);
	void setAttribute(const String& name, const String& value);

	void PaintText(HDC hDC);

    void SetEnabledEffect(bool _EnabledEffect);
    bool GetEnabledEffect();
    void setText(const String& pstrText, const bool needTranslate = true);
    String getText() const;
    void SetTransShadow(int _TransShadow);
    int GetTransShadow();
    void SetTransShadow1(int _TransShadow);
    int GetTransShadow1();
    void SetTransText(int _TransText);
    int GetTransText();
    void SetTransText1(int _TransText);
    int GetTransText1();
    void SetTransStroke(int _TransStroke);
    int GetTransStroke();
    void SetGradientLength(int _GradientLength);
    int GetGradientLength();
    void SetTextRenderingHintAntiAlias(int _TextRenderingHintAntiAlias);
    int GetTextRenderingHintAntiAlias();
    void SetShadowOffset(int _offset,int _angle);
	Gdiplus::RectF GetShadowOffset();
    void SetTextColor1(DWORD _TextColor1);
    DWORD GetTextColor1();
    void SetTextShadowColorA(DWORD _TextShadowColorA);
    DWORD GetTextShadowColorA();
    void SetTextShadowColorB(DWORD _TextShadowColorB);
    DWORD GetTextShadowColorB();
    void SetStrokeColor(DWORD _StrokeColor);
    DWORD GetStrokeColor();
    void SetGradientAngle(int _SetGradientAngle);
    int GetGradientAngle();
    void SetEnabledStroke(bool _EnabledStroke);
    bool GetEnabledStroke();
    void SetEnabledShadow(bool _EnabledShadowe);
    bool GetEnabledShadow();

protected:
	uint32_t _textColor;
	uint32_t _disabledTextColor;
	int m_iFont;
	UINT _uTextStyle;
	RECT _rcTextPadding;
	bool m_bShowHtml;

    int m_hAlign;
    int m_vAlign;
    int m_TransShadow;
    int m_TransShadow1;
    int m_TransText;
    int m_TransText1;
    int m_TransStroke;
    int m_GradientLength;
    int m_GradientAngle;
    bool _enableEffect;
    bool m_EnabledStroke;
    bool m_EnabledShadow;
    DWORD m_dwTextColor1;
    DWORD m_dwTextShadowColorA;
    DWORD m_dwTextShadowColorB;
    DWORD m_dwStrokeColor;
    Gdiplus::RectF m_ShadowOffset;
    String m_TextValue;
    Gdiplus::GdiplusStartupInput m_gdiplusStartupInput;
    Gdiplus::TextRenderingHint m_TextRenderingHintAntiAlias;

private:
	static const String CLASS_NAME;
};

}

#endif // ZGUI_USE_LABEL

#endif // __ZGUI_LABEL_H_