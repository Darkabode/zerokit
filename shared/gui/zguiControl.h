#ifndef __ZGUI_CONTROL_H_
#define __ZGUI_CONTROL_H_

namespace zgui {

typedef Control* (CALLBACK* FINDCONTROLPROC)(Control*, LPVOID);

class Control
{
public:
    Control();
    virtual ~Control();

public:
    virtual String GetName() const;
    virtual void SetName(const String& name);
    virtual const String& getClass() const;
    virtual LPVOID getInterface(const String& name);
    virtual UINT GetControlFlags() const;

    virtual bool Activate();
	virtual PaintManager* getManager() const { return _pManager; }
    virtual void SetManager(PaintManager* pManager, Control* pParent, bool bInit = true);
    virtual Control* GetParent() const;

    virtual String getText() const;
    virtual void setText(const String& pstrText, const bool needTranslate = true);
	virtual void updateText();

    uint32_t GetBkColor() const;
    void SetBkColor(uint32_t dwBackColor);
    uint32_t GetBkColor2() const;
    void SetBkColor2(uint32_t dwBackColor);
    uint32_t GetBkColor3() const;
    void SetBkColor3(uint32_t dwBackColor);
    const String& GetBkImage();
    void SetBkImage(const String& imageName);
	uint32_t GetFocusBorderColor() const;
	void SetFocusBorderColor(uint32_t dwBorderColor);
    bool IsColorHSL() const;
    void SetColorHSL(bool bColorHSL);
    SIZE GetBorderRound() const;
    void SetBorderRound(SIZE cxyRound);
    bool DrawImage(HDC hDC, const String& pStrImage, const String& pStrModify = String::empty);

    int GetBorderSize() const;
    void SetBorderSize(int nSize);
    uint32_t GetBorderColor() const;
    void SetBorderColor(uint32_t dwBorderColor);
	uint32_t GetDisabledBorderColor() const;
	void SetDisabledBorderColor(uint32_t dwBorderColor);

    void SetBorderSize(RECT rc);
    int GetLeftBorderSize() const;
    void SetLeftBorderSize(int nSize);
    int GetTopBorderSize() const;
    void SetTopBorderSize(int nSize);
    int GetRightBorderSize() const;
    void SetRightBorderSize(int nSize);
    int GetBottomBorderSize() const;
    void SetBottomBorderSize(int nSize);
    int GetBorderStyle() const;
    void SetBorderStyle(int nStyle);

    virtual const RECT& GetPos() const;
    virtual void SetPos(RECT rc);
    virtual int GetWidth() const;
    virtual int GetHeight() const;
    virtual int GetX() const;
    virtual int GetY() const;
    virtual RECT GetPadding() const;
    virtual void SetPadding(RECT rcPadding);
    virtual SIZE GetFixedXY() const;
    virtual void SetFixedXY(SIZE szXY);
    virtual int GetFixedWidth() const;
    virtual void SetFixedWidth(int cx);
    virtual int GetFixedHeight() const;
    virtual void SetFixedHeight(int cy);
    virtual int GetMinWidth() const;
    virtual void SetMinWidth(int cx);
    virtual int GetMaxWidth() const;
    virtual void SetMaxWidth(int cx);
    virtual int GetMinHeight() const;
    virtual void SetMinHeight(int cy);
    virtual int GetMaxHeight() const;
    virtual void SetMaxHeight(int cy);
    virtual void SetRelativePos(SIZE szMove,SIZE szZoom);
    virtual void SetRelativeParentSize(SIZE sz);
    virtual TRelativePosUI GetRelativePos() const;
    virtual bool IsRelativePos() const;

    virtual const String& GetToolTip() const;
    virtual void SetToolTip(const String& pstrText);
	virtual void SetToolTipWidth(int nWidth);
	virtual int GetToolTipWidth(void);

    virtual TCHAR GetShortcut() const;
    virtual void SetShortcut(TCHAR ch);

    virtual bool IsContextMenuUsed() const;
    virtual void SetContextMenuUsed(bool bMenuUsed);

    virtual const String& GetUserData();
    virtual void SetUserData(const String& pstrText);
    virtual UINT_PTR GetTag() const;
    virtual void SetTag(UINT_PTR pTag);

#ifdef ZGUI_USE_ANIMATION
	virtual void setEffectsStyle(const String& effectStyle, TEffectAge* pTEffectAge = 0);
	virtual void triggerEffects(TEffectAge* pTEffectAge = 0);
	Animation& getAnimation() { return _anim; }
#endif // ZGUI_USE_ANIMATION

    virtual bool IsVisible() const;
    virtual void SetVisible(bool bVisible = true);
    virtual void SetInternVisible(bool bVisible = true);
    virtual bool IsEnabled() const;
    virtual void SetEnabled(bool bEnable = true);
    virtual bool IsMouseEnabled() const;
    virtual void SetMouseEnabled(bool bEnable = true);
    virtual bool IsKeyboardEnabled() const;
    virtual void SetKeyboardEnabled(bool bEnable = true);
    virtual bool IsFocused() const;
    virtual void SetFocus();
    virtual bool IsFloat() const;
    virtual void SetFloat(bool bFloat = true);

    virtual Control* FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags);

    void Invalidate();
    bool IsUpdateNeeded() const;
    void NeedUpdate();
    void NeedParentUpdate();
    uint32_t GetAdjustColor(uint32_t dwColor);

    virtual void Init();
    virtual void DoInit();

    virtual void Event(TEventUI& event);
    virtual void DoEvent(TEventUI& event);

    virtual void setAttribute(const String& pstrName, const String& pstrValue);
    Control* applyAttributeList(const String& pstrList);

    virtual SIZE EstimateSize(SIZE szAvailable);

    virtual void DoPaint(HDC hDC, const RECT& rcPaint);
    virtual void PaintBkColor(HDC hDC);
    virtual void PaintBkImage(HDC hDC);
    virtual void PaintStatusImage(HDC hDC);
    virtual void PaintText(HDC hDC);
    virtual void PaintBorder(HDC hDC);

    virtual void DoPostPaint(HDC hDC, const RECT& rcPaint);

    void SetVirtualWnd(const String& pstrValue);
    String GetVirtualWnd() const;

public:
    EventSource OnInit;
    EventSource OnDestroy;
    EventSource OnSize;
    EventSource OnEvent;
    EventSource OnNotify;

protected:
    PaintManager* _pManager;
    Control* _pParent;
    String m_sVirtualWnd;
    String _name;
    bool m_bUpdateNeeded;
    bool m_bMenuUsed;
    RECT _rcItem;
    RECT _rcPadding;
    SIZE m_cXY;
    SIZE _cxyFixed;
    SIZE m_cxyMin;
    SIZE m_cxyMax;
    bool m_bVisible;
    bool m_bInternVisible;
    bool m_bEnabled;
    bool m_bMouseEnabled;
	bool m_bKeyboardEnabled ;
    bool m_bFocused;
    bool _bFloat;
    bool m_bSetPos;
    TRelativePosUI m_tRelativePos;

    String _text;
	String _unifiedText;
    String _tooltipText;
    TCHAR m_chShortcut;
    String _userData;
    UINT_PTR m_pTag;

    uint32_t _dwBackColor;
    uint32_t _dwBackColor2;
    uint32_t _dwBackColor3;
    bool _gradientVertical;
    int _gradientSteps;

    String _bkImageName;
    String m_sForeImage;
    uint32_t _dwBorderColor;
	uint32_t _dwDisabledBorderColor;
	uint32_t _dwFocusBorderColor;
    bool m_bColorHSL;
    int m_nBorderSize;
    int m_nBorderStyle;
	int m_nTooltipWidth;
    SIZE m_cxyBorderRound;
    RECT _rcPaint;
    RECT m_rcBorderSize;

#ifdef ZGUI_USE_ANIMATION
	TEffectAge _curEffects;
	Animation _anim;
#endif // ZGUI_USE_ANIMATION

private:
	static const String CLASS_NAME;
};

} // namespace zgui

#endif // __ZGUI_CONTROL_H_
