#ifndef __UICONTROL_H__
#define __UICONTROL_H__

namespace zgui {

typedef CControlUI* (CALLBACK* FINDCONTROLPROC)(CControlUI*, LPVOID);

class CControlUI
{
public:
    CControlUI();
    virtual ~CControlUI();

public:
    virtual String GetName() const;
    virtual void SetName(const String& pstrName);
    virtual LPCTSTR GetClass() const;
    virtual LPVOID GetInterface(LPCTSTR pstrName);
    virtual UINT GetControlFlags() const;

    virtual bool Activate();
    virtual CPaintManagerUI* GetManager() const;
    virtual void SetManager(CPaintManagerUI* pManager, CControlUI* pParent, bool bInit = true);
    virtual CControlUI* GetParent() const;

    // 文本相关
    virtual String GetText() const;
    virtual void SetText(const String& pstrText);

    // 图形相关
    DWORD GetBkColor() const;
    void SetBkColor(DWORD dwBackColor);
    DWORD GetBkColor2() const;
    void SetBkColor2(DWORD dwBackColor);
    DWORD GetBkColor3() const;
    void SetBkColor3(DWORD dwBackColor);
    const String& GetBkImage();
    void SetBkImage(const String& imageName);
	DWORD GetFocusBorderColor() const;
	void SetFocusBorderColor(DWORD dwBorderColor);
    bool IsColorHSL() const;
    void SetColorHSL(bool bColorHSL);
    SIZE GetBorderRound() const;
    void SetBorderRound(SIZE cxyRound);
    bool DrawImage(HDC hDC, const String& pStrImage, const String& pStrModify = String::empty);

    //边框相关
    int GetBorderSize() const;
    void SetBorderSize(int nSize);
    DWORD GetBorderColor() const;
    void SetBorderColor(DWORD dwBorderColor);

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

    // 位置相关
    virtual const RECT& GetPos() const;
    virtual void SetPos(RECT rc);
    virtual int GetWidth() const;
    virtual int GetHeight() const;
    virtual int GetX() const;
    virtual int GetY() const;
    virtual RECT GetPadding() const;
    virtual void SetPadding(RECT rcPadding); // 设置外边距，由上层窗口绘制
    virtual SIZE GetFixedXY() const;         // 实际大小位置使用GetPos获取，这里得到的是预设的参考值
    virtual void SetFixedXY(SIZE szXY);      // 仅float为true时有效
    virtual int GetFixedWidth() const;       // 实际大小位置使用GetPos获取，这里得到的是预设的参考值
    virtual void SetFixedWidth(int cx);      // 预设的参考值
    virtual int GetFixedHeight() const;      // 实际大小位置使用GetPos获取，这里得到的是预设的参考值
    virtual void SetFixedHeight(int cy);     // 预设的参考值
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

    // 鼠标提示
    virtual String GetToolTip() const;
    virtual void SetToolTip(const String& pstrText);

    // 快捷键
    virtual TCHAR GetShortcut() const;
    virtual void SetShortcut(TCHAR ch);

    // 菜单
    virtual bool IsContextMenuUsed() const;
    virtual void SetContextMenuUsed(bool bMenuUsed);

    // 用户属性
    virtual const String& GetUserData();
    virtual void SetUserData(const String& pstrText);
    virtual UINT_PTR GetTag() const;
    virtual void SetTag(UINT_PTR pTag);

    // 一些重要的属性
    virtual bool IsVisible() const;
    virtual void SetVisible(bool bVisible = true);
    virtual void SetInternVisible(bool bVisible = true); // 仅供内部调用，有些UI拥有窗口句柄，需要重写此函数
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

    virtual CControlUI* FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags);

    void Invalidate();
    bool IsUpdateNeeded() const;
    void NeedUpdate();
    void NeedParentUpdate();
    DWORD GetAdjustColor(DWORD dwColor);

    virtual void Init();
    virtual void DoInit();

    virtual void Event(TEventUI& event);
    virtual void DoEvent(TEventUI& event);

    virtual void SetAttribute(const String& pstrName, const String& pstrValue);
    CControlUI* ApplyAttributeList(const String& pstrList);

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
    CEventSource OnInit;
    CEventSource OnDestroy;
    CEventSource OnSize;
    CEventSource OnEvent;
    CEventSource OnNotify;

protected:
    CPaintManagerUI* _pManager;
    CControlUI* _pParent;
    String m_sVirtualWnd;
    String _name;
    bool m_bUpdateNeeded;
    bool m_bMenuUsed;
    RECT _rcItem;
    RECT m_rcPadding;
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
    bool m_bSetPos; // 防止SetPos循环调用
    TRelativePosUI m_tRelativePos;

    String _text;
    String _tooltipText;
    TCHAR m_chShortcut;
    String _userData;
    UINT_PTR m_pTag;

    DWORD _dwBackColor;
    DWORD _dwBackColor2;
    DWORD _dwBackColor3;
    bool _gradientVertical;
    int _gradientSteps;

    String _bkImageName;
    String m_sForeImage;
    DWORD m_dwBorderColor;
	DWORD m_dwFocusBorderColor;
    bool m_bColorHSL;
    int m_nBorderSize;
    int m_nBorderStyle;
    SIZE m_cxyBorderRound;
    RECT _rcPaint;
    RECT m_rcBorderSize;
};

} // namespace zgui

#endif // __UICONTROL_H__
