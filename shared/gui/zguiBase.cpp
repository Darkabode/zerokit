#include "zgui.h"

#ifdef _DEBUG
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif

namespace zgui {

void __Trace(LPCWSTR pstrFormat, ...)
{
#ifdef _DEBUG
    wchar_t szBuffer[300];
    va_list args;
    va_start(args, pstrFormat);
    fn_wvnsprintfW(szBuffer, lengthof(szBuffer) - 2, pstrFormat, args);
    fn_lstrcatW(szBuffer, L"\n");
    va_end(args);
    fn_OutputDebugStringW(szBuffer);
#endif
}

#define UILIB_COMDAT __declspec(selectany)

LPCTSTR __TraceMsg(UINT uMsg)
{
#define MSGDEF(x) if(uMsg==x) return L#x
    MSGDEF(WM_SETCURSOR);
    MSGDEF(WM_NCHITTEST);
    MSGDEF(WM_NCPAINT);
    MSGDEF(WM_PAINT);
    MSGDEF(WM_ERASEBKGND);
    MSGDEF(WM_NCMOUSEMOVE);  
    MSGDEF(WM_MOUSEMOVE);
    MSGDEF(WM_MOUSELEAVE);
    MSGDEF(WM_MOUSEHOVER);   
    MSGDEF(WM_NOTIFY);
    MSGDEF(WM_COMMAND);
    MSGDEF(WM_MEASUREITEM);
    MSGDEF(WM_DRAWITEM);   
    MSGDEF(WM_LBUTTONDOWN);
    MSGDEF(WM_LBUTTONUP);
    MSGDEF(WM_LBUTTONDBLCLK);
    MSGDEF(WM_RBUTTONDOWN);
    MSGDEF(WM_RBUTTONUP);
    MSGDEF(WM_RBUTTONDBLCLK);
    MSGDEF(WM_SETFOCUS);
    MSGDEF(WM_KILLFOCUS);  
    MSGDEF(WM_MOVE);
    MSGDEF(WM_SIZE);
    MSGDEF(WM_SIZING);
    MSGDEF(WM_MOVING);
    MSGDEF(WM_GETMINMAXINFO);
    MSGDEF(WM_CAPTURECHANGED);
    MSGDEF(WM_WINDOWPOSCHANGED);
    MSGDEF(WM_WINDOWPOSCHANGING);   
    MSGDEF(WM_NCCALCSIZE);
    MSGDEF(WM_NCCREATE);
    MSGDEF(WM_NCDESTROY);
    MSGDEF(WM_TIMER);
    MSGDEF(WM_KEYDOWN);
    MSGDEF(WM_KEYUP);
    MSGDEF(WM_CHAR);
    MSGDEF(WM_SYSKEYDOWN);
    MSGDEF(WM_SYSKEYUP);
    MSGDEF(WM_SYSCOMMAND);
    MSGDEF(WM_SYSCHAR);
    MSGDEF(WM_VSCROLL);
    MSGDEF(WM_HSCROLL);
    MSGDEF(WM_CHAR);
    MSGDEF(WM_SHOWWINDOW);
    MSGDEF(WM_PARENTNOTIFY);
    MSGDEF(WM_CREATE);
    MSGDEF(WM_NCACTIVATE);
    MSGDEF(WM_ACTIVATE);
    MSGDEF(WM_ACTIVATEAPP);   
    MSGDEF(WM_CLOSE);
    MSGDEF(WM_DESTROY);
    MSGDEF(WM_GETICON);   
    MSGDEF(WM_GETTEXT);
    MSGDEF(WM_GETTEXTLENGTH);   
    static TCHAR szMsg[10];
    fn_wsprintfW(szMsg, L"0x%04X", uMsg);
    return szMsg;
}


ZGUI_BASE_BEGIN_MESSAGE_MAP(CNotifyPump)
ZGUI_END_MESSAGE_MAP()

static const ZGUI_MSGMAP_ENTRY* DuiFindMessageEntry(const ZGUI_MSGMAP_ENTRY* lpEntry,TNotifyUI& msg )
{
    String sMsgType = msg.sType;
    String sCtrlName = msg.pSender->GetName();
    const ZGUI_MSGMAP_ENTRY* pMsgTypeEntry = NULL;
    while (lpEntry->nSig != ZGuiSig_end) {
        if(lpEntry->sMsgType==sMsgType) {
            if(!lpEntry->sCtrlName.isEmpty()) {
                if(lpEntry->sCtrlName==sCtrlName) {
                    return lpEntry;
                }
            }
            else {
                pMsgTypeEntry = lpEntry;
            }
        }
        lpEntry++;
    }
    return pMsgTypeEntry;
}

bool CNotifyPump::AddVirtualWnd(const String& strName,CNotifyPump* pObject)
{
    if (m_VirtualWndMap.Find(strName) == NULL) {
        m_VirtualWndMap.Insert(strName, (LPVOID)pObject);
        return true;
    }
    return false;
}

bool CNotifyPump::RemoveVirtualWnd(const String& strName)
{
    if (m_VirtualWndMap.Find(strName) != NULL) {
        m_VirtualWndMap.Remove(strName);
        return true;
    }
    return false;
}

bool CNotifyPump::LoopDispatch(TNotifyUI& msg)
{
    const ZGUI_MSGMAP_ENTRY* lpEntry = NULL;
    const ZGUI_MSGMAP* pMessageMap = NULL;

    for(pMessageMap = GetMessageMap(); pMessageMap!=NULL; pMessageMap = pMessageMap->pBaseMap) {
        zgui_assert(pMessageMap != pMessageMap->pBaseMap);
        if ((lpEntry = DuiFindMessageEntry(pMessageMap->lpEntries,msg)) != NULL)
        {
            goto LDispatch;
        }
    }
    return false;

LDispatch:
    union ZGuiMessageMapFunctions mmf;
    mmf.pfn = lpEntry->pfn;

    bool bRet = false;
    int nSig;
    nSig = lpEntry->nSig;
    switch (nSig)
    {
    default:
        zgui_assert(FALSE);
        break;
    case ZGuiSig_lwl:
        (this->*mmf.pfn_Notify_lwl)(msg.wParam,msg.lParam);
        bRet = true;
        break;
    case ZGuiSig_vn:
        (this->*mmf.pfn_Notify_vn)(msg);
        bRet = true;
        break;
    }
    return bRet;
}

void CNotifyPump::NotifyPump(TNotifyUI& msg)
{
    ///遍历虚拟窗口
    if (!msg.sVirtualWnd.isEmpty()) {
        for (int i = 0; i< m_VirtualWndMap.GetSize(); ++i) {
			const String& key = m_VirtualWndMap.GetAt(i);
			if (!key.isEmpty()) {
                if (msg.sVirtualWnd.containsIgnoreCase(String(key))) {
                    CNotifyPump* pObject = static_cast<CNotifyPump*>(m_VirtualWndMap.Find(key, false));
                    if (pObject && pObject->LoopDispatch(msg)) {
                        return;
                    }
                }
            }
        }
    }

    ///
    //遍历主窗口
    LoopDispatch( msg );
}

Window::Window() :
_hWnd(0),
_oldWndProc((WNDPROC)fn_DefWindowProcW),
_bSubclassed(false)
{
}

Window::~Window()
{
    if (_hWnd != 0) {
        ActiveWindows::getInstance()->remove(_hWnd);
    }
}

HWND Window::getHWND() const throw()
{ 
    return _hWnd; 
}

UINT Window::GetClassStyle() const throw()
{
    return CS_OWNDC;
}

LPCTSTR Window::GetSuperClassName() const throw()
{
    return NULL;
}

Window::operator HWND() const throw()
{
    return _hWnd;
}

HWND Window::CreateDuiWindow( HWND hwndParent, LPCTSTR pstrWindowName,DWORD dwStyle /*=0*/, DWORD dwExStyle /*=0*/ )
{
	return Create(hwndParent,pstrWindowName,dwStyle,dwExStyle,0,0,0,0,NULL);
}

HWND Window::Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, const RECT rc, HMENU hMenu)
{
    return Create(hwndParent, pstrName, dwStyle, dwExStyle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hMenu);
}

// #include <strsafe.h>
// void ErrorExit(LPTSTR lpszFunction) 
// { 
//     // Retrieve the system error message for the last-error code
// 
//     LPVOID lpMsgBuf;
//     LPVOID lpDisplayBuf;
//     DWORD dw = fn_GetLastError(); 
// 
//     FormatMessageW(
//         FORMAT_MESSAGE_ALLOCATE_BUFFER | 
//         FORMAT_MESSAGE_FROM_SYSTEM |
//         FORMAT_MESSAGE_IGNORE_INSERTS,
//         NULL,
//         dw,
//         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//         (LPTSTR) &lpMsgBuf,
//         0, NULL );
// 
//     // Display the error message and exit the process
// 
//     lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
//         (lstrlen((LPCTSTR)lpMsgBuf)+lstrlenW((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
//     StringCchPrintfW((LPTSTR)lpDisplayBuf, 
//         LocalSize(lpDisplayBuf) / sizeof(TCHAR),
//         TEXT("%s failed with error %d: %s"), 
//         lpszFunction, dw, lpMsgBuf); 
//     //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 
// 
//     LocalFree(lpMsgBuf);
//     LocalFree(lpDisplayBuf);
//     ExitProcess(dw); 
// }



HWND Window::Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x, int y, int cx, int cy, HMENU hMenu)
{
    if (GetSuperClassName() != NULL && !RegisterSuperclass()) {
        return NULL;
    }

    if (GetSuperClassName() == NULL && !RegisterWindowClass()) {
        return NULL;
    }

    _hWnd = fn_CreateWindowExW(dwExStyle, GetWindowClassName().toWideCharPointer(), pstrName, dwStyle, x, y, cx, cy, hwndParent, hMenu, PaintManager::GetInstance(), this);
    //ErrorExit(L"SDfsdf");
    zgui_assert(_hWnd!=NULL);

    if (_hWnd != 0) {
        ActiveWindows::getInstance()->add(_hWnd);
    }
    return _hWnd;
}

HWND Window::Subclass(HWND hWnd)
{
    zgui_assert(fn_IsWindow(hWnd));
    zgui_assert(_hWnd==NULL);
    _oldWndProc = (WNDPROC)fn_SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)__WndProc);
    if (_oldWndProc == NULL) {
        return NULL;
    }
    _bSubclassed = true;
    _hWnd = hWnd;
    ::fn_SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(this));
    return _hWnd;
}

void Window::Unsubclass()
{
    zgui_assert(fn_IsWindow(_hWnd));
    if (!fn_IsWindow(_hWnd)) {
        return;
    }
    if (!_bSubclassed) {
        return;
    }
    fn_SetWindowLongPtrW(_hWnd, GWLP_WNDPROC, (LONG_PTR)_oldWndProc);
    _oldWndProc = (WNDPROC)fn_DefWindowProcW;
    _bSubclassed = false;
}

void Window::ShowWindow(bool bShow /*= true*/, bool bTakeFocus /*= false*/)
{
    zgui_assert(fn_IsWindow(_hWnd));
    if (!fn_IsWindow(_hWnd)) {
        return;
    }
    fn_ShowWindow(_hWnd, bShow ? (bTakeFocus ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE) : SW_HIDE);
}

UINT Window::ShowModal()
{
    zgui_assert(fn_IsWindow(_hWnd));
    UINT nRet = 0;
    HWND hWndParent = fn_GetWindow(_hWnd, GW_OWNER);
    fn_ShowWindow(_hWnd, SW_SHOWNORMAL);
    fn_EnableWindow(hWndParent, FALSE);
    MSG msg;
    __stosb((uint8_t*)&msg, 0, sizeof(msg));
    while (fn_IsWindow(_hWnd) && fn_GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_CLOSE && msg.hwnd == _hWnd) {
            nRet = msg.wParam;
            fn_EnableWindow(hWndParent, TRUE);
            fn_SetFocus(hWndParent);
        }
        if (!PaintManager::TranslateMessage(&msg)) {
            fn_TranslateMessage(&msg);
            fn_DispatchMessageW(&msg);
        }
        if (msg.message == WM_QUIT) {
            break;
        }
    }
    fn_EnableWindow(hWndParent, TRUE);
    fn_SetFocus(hWndParent);
    if (msg.message == WM_QUIT) {
        fn_PostQuitMessage(msg.wParam);
    }
    return nRet;
}

void Window::Close(UINT nRet)
{
    zgui_assert(fn_IsWindow(_hWnd));
    if (!fn_IsWindow(_hWnd)) {
        return;
    }
    postMessage(WM_CLOSE, (WPARAM)nRet, 0L);
}

void Window::centerWindow()
{
    zgui_assert(fn_IsWindow(_hWnd));
    zgui_assert((fn_GetWindowLongW(_hWnd, GWL_STYLE) & WS_CHILD)==0);
    RECT rcDlg;
    __stosb((uint8_t*)&rcDlg, 0, sizeof(rcDlg));
    fn_GetWindowRect(_hWnd, &rcDlg);
    RECT rcArea;
    RECT rcCenter;
	HWND hWnd = *this;
    __stosb((uint8_t*)&rcArea, 0, sizeof(rcArea));
    __stosb((uint8_t*)&rcCenter, 0, sizeof(rcCenter));
    HWND hWndCenter = fn_GetWindow(_hWnd, GW_OWNER);
	if (hWndCenter != 0) {
		hWnd=hWndCenter;
	}

	MONITORINFO oMonitor;
	__stosb((uint8_t*)&oMonitor, 0, sizeof(oMonitor));
	oMonitor.cbSize = sizeof(oMonitor);
	fn_GetMonitorInfoW(fn_MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &oMonitor);
	rcArea = oMonitor.rcWork;

	if (hWndCenter == 0) {
		rcCenter = rcArea;
	}
	else {
		fn_GetWindowRect(hWndCenter, &rcCenter);
	}

    int DlgWidth = rcDlg.right - rcDlg.left;
    int DlgHeight = rcDlg.bottom - rcDlg.top;

    // Find dialog's upper left based on rcCenter
    int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
    int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

    // The dialog is outside the screen, move it inside
    if (xLeft < rcArea.left) {
        xLeft = rcArea.left;
    }
    else if (xLeft + DlgWidth > rcArea.right) {
        xLeft = rcArea.right - DlgWidth;
    }
    if (yTop < rcArea.top) {
        yTop = rcArea.top;
    }
    else if (yTop + DlgHeight > rcArea.bottom) {
        yTop = rcArea.bottom - DlgHeight;
    }
	setPosition(xLeft, yTop);
}

void Window::setPosition(int x, int y)
{
	fn_SetWindowPos(_hWnd, NULL, x, y, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::SetIcon(UINT nRes)
{
	HICON hIcon = (HICON)fn_LoadImageW(PaintManager::GetInstance(), MAKEINTRESOURCE(nRes), IMAGE_ICON, (fn_GetSystemMetrics(SM_CXICON) + 15) & ~15, (fn_GetSystemMetrics(SM_CYICON) + 15) & ~15,	LR_DEFAULTCOLOR);
	zgui_assert(hIcon);
	fn_SendMessageW(_hWnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	hIcon = (HICON)fn_LoadImageW(PaintManager::GetInstance(), MAKEINTRESOURCE(nRes), IMAGE_ICON, (fn_GetSystemMetrics(SM_CXICON) + 15) & ~15, (fn_GetSystemMetrics(SM_CYICON) + 15) & ~15, LR_DEFAULTCOLOR);
	zgui_assert(hIcon);
	fn_SendMessageW(_hWnd, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);
}

bool Window::RegisterWindowClass()
{
    WNDCLASSEXW wc;
    __stosb((uint8_t*)&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = GetClassStyle();
    wc.cbWndExtra = 32;
    wc.lpfnWndProc = Window::__WndProc;
    wc.hInstance = PaintManager::GetInstance();
    wc.hCursor = fn_LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = GetWindowClassName().toWideCharPointer();
    ATOM ret = fn_RegisterClassExW(&wc);
    zgui_assert(ret!=NULL || fn_GetLastError()==ERROR_CLASS_ALREADY_EXISTS);
    return ret != NULL || fn_GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool Window::RegisterSuperclass()
{
    // Get the class information from an existing
    // window so we can subclass it later on...
    WNDCLASSEX wc;
    __stosb((uint8_t*)&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!fn_GetClassInfoExW(NULL, GetSuperClassName(), &wc)) {
        if (!fn_GetClassInfoExW(PaintManager::GetInstance(), GetSuperClassName(), &wc)) {
            zgui_assert(!"Unable to locate window class");
            return NULL;
        }
    }
    _oldWndProc = wc.lpfnWndProc;
    wc.lpfnWndProc = Window::__ControlProc;
    wc.hInstance = PaintManager::GetInstance();
    wc.lpszClassName = GetWindowClassName().toWideCharPointer();
    ATOM ret = fn_RegisterClassExW(&wc);
    zgui_assert(ret!=NULL || fn_GetLastError()==ERROR_CLASS_ALREADY_EXISTS);
    return ret != NULL || fn_GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

LRESULT CALLBACK Window::__WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* pThis = NULL;
    if (uMsg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<Window*>(lpcs->lpCreateParams);
        pThis->_hWnd = hWnd;
        fn_SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pThis));
    } 
    else {
        pThis = reinterpret_cast<Window*>(fn_GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (uMsg == WM_NCDESTROY && pThis != NULL) {
            LRESULT lRes = fn_CallWindowProcW(pThis->_oldWndProc, hWnd, uMsg, wParam, lParam);
            fn_SetWindowLongPtrW(pThis->_hWnd, GWLP_USERDATA, 0L);
            if (pThis->_bSubclassed) {
                pThis->Unsubclass();
            }
            ActiveWindows::getInstance()->remove(pThis->_hWnd);
            pThis->_hWnd = 0;
            pThis->OnFinalMessage(hWnd);
            return lRes;
        }
    }
    if (pThis != NULL) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    } 
    else {
        return fn_DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK Window::__ControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* pThis = NULL;
    if( uMsg == WM_NCCREATE ) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<Window*>(lpcs->lpCreateParams);
        fn_SetPropW(hWnd, L"WndX", (HANDLE)pThis);
        pThis->_hWnd = hWnd;
    } 
    else {
        pThis = reinterpret_cast<Window*>(fn_GetPropW(hWnd, L"WndX"));
        if (uMsg == WM_NCDESTROY && pThis != 0) {
            LRESULT lRes = fn_CallWindowProcW(pThis->_oldWndProc, hWnd, uMsg, wParam, lParam);
            if( pThis->_bSubclassed ) pThis->Unsubclass();
            fn_SetPropW(hWnd, L"WndX", 0);
            pThis->_hWnd = 0;
            pThis->OnFinalMessage(hWnd);
            return lRes;
        }
    }
    if (pThis != NULL) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    } 
    else {
        return fn_DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

LRESULT Window::sendMessage(UINT uMsg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
    zgui_assert(fn_IsWindow(_hWnd));
    return fn_SendMessageW(_hWnd, uMsg, wParam, lParam);
} 

LRESULT Window::postMessage(UINT uMsg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
    zgui_assert(fn_IsWindow(_hWnd));
    return fn_PostMessageW(_hWnd, uMsg, wParam, lParam);
}

void Window::ResizeClient(int cx /*= -1*/, int cy /*= -1*/)
{
    zgui_assert(fn_IsWindow(_hWnd));
    RECT rc;
    __stosb((uint8_t*)&rc, 0, sizeof(rc));
    if (!fn_GetClientRect(_hWnd, &rc)) {
        return;
    }
    if (cx != -1) {
        rc.right = cx;
    }
    if (cy != -1) {
        rc.bottom = cy;
    }
    if (!fn_AdjustWindowRectEx(&rc, fn_GetWindowLongW(_hWnd, GWL_STYLE), (!(fn_GetWindowLongW(_hWnd, GWL_STYLE) & WS_CHILD) && (fn_GetMenu(_hWnd) != NULL)), fn_GetWindowLongW(_hWnd, GWL_EXSTYLE))) {
        return;
    }
    fn_SetWindowPos(_hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return fn_CallWindowProcW(_oldWndProc, _hWnd, uMsg, wParam, lParam);
}

void Window::OnFinalMessage(HWND /*hWnd*/)
{
}

ZGUI_IMPLEMENT_SINGLETON(ActiveWindows);

ActiveWindows::ActiveWindows()
{
}

ActiveWindows::~ActiveWindows()
{
}

void ActiveWindows::add(HWND hWnd)
{
    const ScopedLock lock(_critSect);
    _windows.add(hWnd);
}

void ActiveWindows::remove(HWND hWnd)
{
    const ScopedLock lock(_critSect);
    _windows.removeAllInstancesOf(hWnd);
}

bool ActiveWindows::contains(HWND hWnd)
{
    const ScopedLock lock(_critSect);
    return _windows.contains(hWnd);
}


const String TimerWindow::WINDOW_CLASS_NAME = "TimerWnd";

TimerWindow::TimerWindow()
{
	m_TimerStarted = false;
	m_TimerTick = NULL;
	m_lParam = 0;
}

TimerWindow::~TimerWindow()
{
	killTimer();
}

const String& TimerWindow::GetWindowClassName() const
{
	return WINDOW_CLASS_NAME;
}

bool TimerWindow::setTimer(UINT interval, pfnTimerTick fnTimerTick, LPARAM lParam)
{
	if (EnsureHandle()) {
		if (m_TimerStarted) {
			killTimer();
			m_TimerStarted = false;
		}

		if (fn_SetTimer(getHWND(), 0, interval, NULL)) {
			m_TimerStarted = true;
		}

		m_TimerTick = fnTimerTick;
		m_lParam = lParam;
	}
	return m_TimerStarted;
}

void TimerWindow::killTimer()
{
	if (m_TimerStarted) {
		fn_KillTimer(getHWND(), 0);
		m_TimerStarted = false;
		m_TimerTick = NULL;
		m_lParam = 0;
	}
}
LRESULT TimerWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_TIMER) {
		if (m_TimerStarted && m_TimerTick) {
			m_TimerTick(m_lParam);
		}
		return 0;
	}
	else {
		return Window::HandleMessage(uMsg, wParam, lParam);
	}
}

bool TimerWindow::HandleCreated()
{
	return getHWND() != 0;
}
void TimerWindow::CreateNativateWindow()
{
	if (!HandleCreated()) {
		Create(HWND_MESSAGE, L"", 0, 0, 0, 0, 0, 0, NULL);
	}
}

bool TimerWindow::EnsureHandle()
{
	if (!HandleCreated()) {
		CreateNativateWindow();
	}
	return HandleCreated();
}



} // namespace zgui
