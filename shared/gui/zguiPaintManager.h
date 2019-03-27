#ifndef __ZGUI_PAINTMANAGER_H_
#define __ZGUI_PAINTMANAGER_H_

namespace zgui {

class Control;

typedef enum EVENTTYPE_UI
{
    UIEVENT__FIRST = 1,
    UIEVENT__KEYBEGIN,
    UIEVENT_KEYDOWN,
    UIEVENT_KEYUP,
    UIEVENT_CHAR,
    UIEVENT_SYSKEY,
    UIEVENT__KEYEND,
    UIEVENT__MOUSEBEGIN,
    UIEVENT_MOUSEMOVE,
    UIEVENT_MOUSELEAVE,
    UIEVENT_MOUSEENTER,
    UIEVENT_MOUSEHOVER,
    UIEVENT_BUTTONDOWN,
    UIEVENT_BUTTONUP,
    UIEVENT_RBUTTONDOWN,
    UIEVENT_DBLCLICK,
    UIEVENT_CONTEXTMENU,
    UIEVENT_SCROLLWHEEL,
    UIEVENT__MOUSEEND,
    UIEVENT_KILLFOCUS,
    UIEVENT_SETFOCUS,
    UIEVENT_WINDOWSIZE,
    UIEVENT_SETCURSOR,
    UIEVENT_TIMER,
    UIEVENT_NOTIFY,
    UIEVENT_COMMAND,
    UIEVENT__LAST,
};

/////////////////////////////////////////////////////////////////////////////////////
//

// Flags for Control::GetControlFlags()
#define UIFLAG_TABSTOP       0x00000001
#define UIFLAG_SETCURSOR     0x00000002
#define UIFLAG_WANTRETURN    0x00000004

// Flags for FindControl()
#define UIFIND_ALL           0x00000000
#define UIFIND_VISIBLE       0x00000001
#define UIFIND_ENABLED       0x00000002
#define UIFIND_HITTEST       0x00000004
#define UIFIND_TOP_FIRST     0x00000008
#define UIFIND_ME_FIRST      0x80000000

// Flags for the CDialogLayout stretching
#define UISTRETCH_NEWGROUP   0x00000001
#define UISTRETCH_NEWLINE    0x00000002
#define UISTRETCH_MOVE_X     0x00000004
#define UISTRETCH_MOVE_Y     0x00000008
#define UISTRETCH_SIZE_X     0x00000010
#define UISTRETCH_SIZE_Y     0x00000020

// Flags used for controlling the paint
#define UISTATE_FOCUSED      0x00000001
#define UISTATE_SELECTED     0x00000002
#define UISTATE_DISABLED     0x00000004
#define UISTATE_HOT          0x00000008
#define UISTATE_PUSHED       0x00000010
#define UISTATE_READONLY     0x00000020
#define UISTATE_CAPTURED     0x00000040

struct TFontInfo
{
    TFontInfo()
    {

    }

    ~TFontInfo()
    {

    }

    HFONT hFont;
    String sFontName;
    int iSize;
    bool bBold;
    bool bUnderline;
    bool bItalic;
    TEXTMETRIC tm;
};

struct TImageInfo
{
    HBITMAP hBitmap;
    int width;
    int height;
};

// Structure for notifications from the system to the control implementation.
struct TEventUI
{
    int Type;
    Control* pSender;
    DWORD dwTimestamp;
    POINT ptMouse;
    TCHAR chKey;
    WORD wKeyState;
    WPARAM wParam;
    LPARAM lParam;
} ;

// Structure for relative position to the parent
struct TRelativePosUI
{
	bool bRelative;
	SIZE szParent;
	int nMoveXPercent;
	int nMoveYPercent;
	int nZoomXPercent;
	int nZoomYPercent;
};

// Listener interface
class INotifyUI
{
public:
    virtual void Notify(TNotifyUI& msg) = 0;
};

// MessageFilter interface
class IMessageFilterUI
{
public:
    virtual LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) = 0;
};

class ITranslateAccelerator
{
public:
	virtual LRESULT TranslateAccelerator(MSG *pMsg) = 0;
};


typedef Control* (*LPCREATECONTROL)(LPCTSTR pstrType);

typedef struct tagTIMERINFO
{
	Control* pSender;
	UINT nLocalID;
	HWND hWnd;
	UINT uWinTimer;
	bool bKilled;
} TIMERINFO;

class PaintManager
{
public:
    PaintManager();
    ~PaintManager();

    void Init(HWND hWnd);
    void NeedUpdate();
    void Invalidate(RECT& rcItem);

    HDC GetPaintDC() const;
    HWND GetPaintWindow() const;
    HWND GetTooltipWindow() const;

    POINT GetMousePos() const;
    SIZE GetClientSize() const;
    SIZE GetInitSize();
    void SetInitSize(int cx, int cy);
    RECT& GetSizeBox();
    void SetSizeBox(RECT& rcSizeBox);
    RECT& GetCaptionRect();
    void SetCaptionRect(RECT& rcCaption);
    SIZE GetRoundCorner() const;
    void SetRoundCorner(int cx, int cy);
    SIZE GetMinInfo() const;
    void SetMinInfo(int cx, int cy);
    SIZE GetMaxInfo() const;
    void SetMaxInfo(int cx, int cy);
	int GetTransparent() const;
    void SetTransparent(int nOpacity);
    void SetBackgroundTransparent(bool bTrans);
    bool IsShowUpdateRect() const;
    void SetShowUpdateRect(bool show);
    LPRECT getMonitorArea();

    static HINSTANCE GetInstance();
    static String GetInstancePath();
    static String GetCurrentPath();
    static void SetInstance(HINSTANCE hInst);
    static void SetCurrentPath(LPCWSTR pStrPath);
    static void GetHSL(short* H, short* S, short* L);
    static void SetHSL(bool bUseHSL, short H, short S, short L); // H:0~360, S:0~200, L:0~200 
    static void ReloadSkin();
    static bool LoadPlugin(LPCTSTR pstrModuleName);
    static Array<void*>* GetPlugins();
    

    bool UseParentResource(PaintManager* pm);
    PaintManager* GetParentResource() const;

    DWORD GetDefaultDisabledColor() const;
    void SetDefaultDisabledColor(DWORD dwColor);
    DWORD GetDefaultFontColor() const;
    void SetDefaultFontColor(DWORD dwColor);
    DWORD GetDefaultLinkFontColor() const;
    void SetDefaultLinkFontColor(DWORD dwColor);
    DWORD GetDefaultLinkHoverFontColor() const;
    void SetDefaultLinkHoverFontColor(DWORD dwColor);
    DWORD GetDefaultSelectedBkColor() const;
    void SetDefaultSelectedBkColor(DWORD dwColor);
    TFontInfo* GetDefaultFontInfo();
    void SetDefaultFont(const String& fontName, int nSize, bool bBold, bool bUnderline, bool bItalic);
    DWORD GetCustomFontCount() const;
    HFONT AddFont(const String& pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic);
    HFONT AddFontAt(int index, LPCTSTR pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic);
    HFONT GetFont(int index);
    HFONT GetFont(const String& pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic);
    bool FindFont(HFONT hFont);
    bool FindFont(const String& pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic);
    int GetFontIndex(HFONT hFont);
    int GetFontIndex(const String& pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic);
    bool RemoveFont(HFONT hFont);
    bool RemoveFontAt(int index);
    void RemoveAllFonts();
    TFontInfo* GetFontInfo(int index);
    TFontInfo* GetFontInfo(HFONT hFont);

    const TImageInfo* getImage(const String& bitmap);
    const TImageInfo* getImageEx(const String& bitmap);
    const TImageInfo* addImage(const String& bitmap);
    const TImageInfo* addImage(const String& bitmap, HBITMAP hBitmap, int iWidth, int iHeight);
    bool removeImage(const String& bitmap);
    void removeAllImages();
    void reloadAllImages();

    void addDefaultAttributeList(const String& controlName, const String& controlAttrList);
    const String& getDefaultAttributeList(const String& controlName) const;
    bool RemoveDefaultAttributeList(LPCTSTR pStrControlName);
    const StringPtrMap& GetDefaultAttribultes() const;
    void RemoveAllDefaultAttributeList();

#ifdef ZGUI_USE_ANIMATION
	bool addAnimationJob(Animation* pAnimation);
#endif // ZGUI_USE_ANIMATION

	void addStyle(const String& styleName, StringPairArray* pParams);
	StringPairArray* getStyle(const String& styleName);
	void acceptStyle(Control* pControl, const String& styleName);
	void removeAllStyles();

    bool attachRootControl(Control* pControl);
    bool initControls(Control* pControl, Control* pParent = NULL);
    void ReapObjects(Control* pControl);

    bool AddOptionGroup(const String& pStrGroupName, Control* pControl);
    Array<void*>* GetOptionGroup(const String& pStrGroupName);
    void RemoveOptionGroup(const String& pStrGroupName, Control* pControl);
    void RemoveAllOptionGroups();

    Control* GetFocus() const;
    void SetFocus(Control* pControl);
    void SetFocusNeeded(Control* pControl);

    bool SetNextTabControl(bool bForward = true);

    bool SetTimer(Control* pControl, UINT nTimerID, UINT uElapse);
    bool KillTimer(Control* pControl, UINT nTimerID);
    void KillTimer(Control* pControl);
    void RemoveAllTimers();

    void SetCapture();
    void ReleaseCapture();
    bool IsCaptured();

    void AddNotifier(INotifyUI* pControl);
    bool RemoveNotifier(INotifyUI* pControl);   
    void SendNotify(TNotifyUI& Msg, bool bAsync = false);
    void SendNotify(Control* pControl, const String& pstrMessage, WPARAM wParam = 0, LPARAM lParam = 0, bool bAsync = false);

    void AddPreMessageFilter(IMessageFilterUI* pFilter);
    bool RemovePreMessageFilter(IMessageFilterUI* pFilter);

    void AddMessageFilter(IMessageFilterUI* pFilter);
    bool RemoveMessageFilter(IMessageFilterUI* pFilter);

    int GetPostPaintCount() const;
    void AddPostPaint(Control* pControl);
    bool RemovePostPaint(Control* pControl);
    void SetPostPaintIndex(Control* pControl, int iIndex);

    void AddDelayedCleanup(Control* pControl);

	void AddTranslateAccelerator(ITranslateAccelerator *pTranslateAccelerator);
	bool RemoveTranslateAccelerator(ITranslateAccelerator *pTranslateAccelerator);
	bool TranslateAccelerator(LPMSG pMsg);

    Control* GetRoot() const;
    Control* FindControl(POINT pt) const;
    Control* FindControl(const String& pstrName) const;
    Control* FindSubControlByPoint(Control* pParent, POINT pt) const;
	Control* FindSubControlByName(Control* pParent, const String& pstrName) const;
	Control* FindSubControlByClass(Control* pParent, const String& pstrClass, int iIndex = 0);
	Array<void*>* FindSubControlsByClass(Control* pParent, const String& pstrClass);
    Array<void*>* GetSubControlsByClass();

    static void messageLoop();
    static bool TranslateMessage(const LPMSG pMsg);
	static void Term();

    bool MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lRes);
    bool PreMessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lRes);
    void UsedVirtualWnd(bool bUsed);
	void HideTooltip();

private:
    static Control* CALLBACK __FindControlFromNameHash(Control* pThis, LPVOID pData);
    static Control* CALLBACK __FindControlFromCount(Control* pThis, LPVOID pData);
    static Control* CALLBACK __FindControlFromPoint(Control* pThis, LPVOID pData);
    static Control* CALLBACK __FindControlFromTab(Control* pThis, LPVOID pData);
    static Control* CALLBACK __FindControlFromShortcut(Control* pThis, LPVOID pData);
    static Control* CALLBACK __FindControlFromUpdate(Control* pThis, LPVOID pData);
    static Control* CALLBACK __FindControlFromName(Control* pThis, LPVOID pData);
    static Control* CALLBACK __FindControlFromClass(Control* pThis, LPVOID pData);
    static Control* CALLBACK __FindControlsFromClass(Control* pThis, LPVOID pData);

    HWND _hwndPaint;
	int _nOpacity;
    HDC _hdcPaint;
    HDC _hdcOffscreen;
    HDC _hdcBackground;
    HBITMAP _hbmpOffscreen;
    HBITMAP _hbmpBackground;
    HWND _hwndTooltip;
    TOOLINFOW _tooltip;
	TimerWindow _tooltipTimerWnd;
	
	bool m_bShowUpdateRect;
    //
    Control* _pRootControl;
    Control* m_pFocus;
    Control* m_pEventHover;
    Control* m_pEventClick;
    Control* m_pEventKey;
    //
    POINT m_ptLastMousePos;
    SIZE m_szMinWindow;
    SIZE m_szMaxWindow;
    SIZE m_szInitWindowSize;
    RECT m_rcSizeBox;
    SIZE m_szRoundCorner;
    RECT m_rcCaption;
    UINT m_uTimerID;
    bool m_bFirstLayout;
    bool m_bUpdateNeeded;
    bool m_bFocusNeeded;
    bool m_bOffscreenPaint;
    bool m_bAlphaBackground;
    bool m_bMouseTracking;
    bool m_bMouseCapture;
    bool m_bUsedVirtualWnd;

    //
	Array<INotifyUI*> _notifiers;
	Array<TIMERINFO*> _timers;
	Array<IMessageFilterUI*> _preMessageFilters;
	Array<IMessageFilterUI*> _messageFilters;
	Array<Control*> _postPaintControls;
	Array<Control*> _delayedCleanup;
	Array<TNotifyUI*> _asyncNotify;
	Array<void*> _foundControls;
    StringPtrMap m_mNameHash;
    StringPtrMap m_mOptionGroup;

	StringPtrMap _stylesMap;
    //
    PaintManager* m_pParentResourcePM;
    DWORD m_dwDefaultDisabledColor;
    DWORD m_dwDefaultFontColor;
    DWORD _dwDefaultLinkFontColor;
    DWORD _dwDefaultLinkHoverFontColor;
    DWORD m_dwDefaultSelectedBkColor;
    TFontInfo m_DefaultFontInfo;
	Array<TFontInfo*> _customFonts;
    RECT _monitorRect;

    StringPtrMap m_mImageHash;
    StringPtrMap m_DefaultAttrHash;

#ifdef ZGUI_USE_ANIMATION
	AnimationSpooler _anim;
#endif // ZGUI_USE_ANIMATION
    
	static HINSTANCE m_hInstance;
    static short m_H;
    static short m_S;
    static short m_L;
	static Array<PaintManager*> _preMessages;

public:
	static String m_pStrDefaultFontName;
	Array<ITranslateAccelerator*> _translateAccelerator;
};

} // namespace zgui

#endif // __ZGUI_PAINTMANAGER_H_
