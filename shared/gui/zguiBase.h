#ifndef __ZGUI_BASE_H_
#define __ZGUI_BASE_H_

namespace zgui {

#define UI_WNDSTYLE_CONTAINER  (0)
#define UI_WNDSTYLE_FRAME      (WS_VISIBLE | WS_OVERLAPPEDWINDOW)
#define UI_WNDSTYLE_CHILD      (WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
#define UI_WNDSTYLE_DIALOG     (WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION | WS_DLGFRAME | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)

#define UI_WNDSTYLE_EX_FRAME   (WS_EX_WINDOWEDGE)
#define UI_WNDSTYLE_EX_DIALOG  (WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME)

#define UI_CLASSSTYLE_CONTAINER  (0)
#define UI_CLASSSTYLE_FRAME      (CS_VREDRAW | CS_HREDRAW)
#define UI_CLASSSTYLE_CHILD      (CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_SAVEBITS)
#define UI_CLASSSTYLE_DIALOG     (CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_SAVEBITS)


/////////////////////////////////////////////////////////////////////////////////////
//
#ifndef ASSERT
#define ASSERT(expr)  _ASSERTE(expr)
#endif

#ifdef _DEBUG
#ifndef TRACE
#define TRACE __Trace
#endif
#define TRACEMSG __TraceMsg
#else
#ifndef TRACE
#define TRACE
#endif
#define TRACEMSG L""
#endif

void __Trace(LPCWSTR pstrFormat, ...);
LPCWSTR __TraceMsg(UINT uMsg);

class CNotifyPump
{
public:
    bool AddVirtualWnd(const String& strName, CNotifyPump* pObject);
    bool RemoveVirtualWnd(const String& strName);
    void NotifyPump(TNotifyUI& msg);
    bool LoopDispatch(TNotifyUI& msg);
    ZGUI_DECLARE_MESSAGE_MAP()

private:
    StringPtrMap m_VirtualWndMap;
};

class Window
{
public:
    Window();
    ~Window();

    HWND getHWND() const  throw();
    operator HWND() const  throw();

    bool RegisterWindowClass();
    bool RegisterSuperclass();

    HWND Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, const RECT rc, HMENU hMenu = NULL);
    HWND Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT, HMENU hMenu = NULL);
    HWND CreateDuiWindow(HWND hwndParent, LPCTSTR pstrWindowName,DWORD dwStyle =0, DWORD dwExStyle =0);
    HWND Subclass(HWND hWnd);
    void Unsubclass();
    void ShowWindow(bool bShow = true, bool bTakeFocus = true);
    UINT ShowModal();
    void Close(UINT nRet = IDOK);
    void centerWindow();
	void setPosition(int x, int y);
    void SetIcon(UINT nRes);

    LRESULT sendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0L);
    LRESULT postMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0L);
    void ResizeClient(int cx = -1, int cy = -1);

protected:
    virtual const String& GetWindowClassName() const = 0;
    virtual LPCTSTR GetSuperClassName() const throw();
    virtual UINT GetClassStyle() const throw();

    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void OnFinalMessage(HWND hWnd);

    static LRESULT CALLBACK __WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK __ControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    HWND _hWnd;
    WNDPROC _oldWndProc;
    bool _bSubclassed;
};

class ActiveWindows
{
public:
    ActiveWindows();
    ~ActiveWindows();

    void add(HWND hWnd);
    void remove(HWND hWnd);

    bool contains(HWND hWnd);

	ZGUI_DECLARE_SINGLETON(ActiveWindows);
private:
    Array<HWND> _windows;
    CriticalSection _critSect;
};


typedef void(*pfnTimerTick)(LPARAM);

class TimerWindow : private Window
{
public:
	TimerWindow();
	~TimerWindow();

	virtual const String& GetWindowClassName() const;

	bool setTimer(UINT interval, pfnTimerTick fnTimerTick, LPARAM lParam);
	void killTimer();
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:
	bool HandleCreated();
	void CreateNativateWindow();
	bool EnsureHandle();

private:
	pfnTimerTick m_TimerTick;
	LPARAM m_lParam;
	bool m_TimerStarted;

	static const String WINDOW_CLASS_NAME;
};

} // namespace zgui

#endif // __ZGUI_BASE_H_
