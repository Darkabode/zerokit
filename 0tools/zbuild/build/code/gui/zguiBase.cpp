#include "zgui.h"

#ifdef _DEBUG
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif

namespace zgui {

void __Trace(LPCTSTR pstrFormat, ...)
{
#ifdef _DEBUG
    TCHAR szBuffer[300] = { 0 };
    va_list args;
    va_start(args, pstrFormat);
    ::wvnsprintf(szBuffer, lengthof(szBuffer) - 2, pstrFormat, args);
    _tcscat(szBuffer, _T("\n"));
    va_end(args);
    ::OutputDebugString(szBuffer);
#endif
}

LPCTSTR __TraceMsg(UINT uMsg)
{
#define MSGDEF(x) if(uMsg==x) return _T(#x)
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
    ::wsprintf(szMsg, _T("0x%04X"), uMsg);
    return szMsg;
}


DUI_BASE_BEGIN_MESSAGE_MAP(CNotifyPump)
DUI_END_MESSAGE_MAP()

static const DUI_MSGMAP_ENTRY* DuiFindMessageEntry(const DUI_MSGMAP_ENTRY* lpEntry,TNotifyUI& msg )
{
    String sMsgType = msg.sType;
    String sCtrlName = msg.pSender->GetName();
    const DUI_MSGMAP_ENTRY* pMsgTypeEntry = NULL;
    while (lpEntry->nSig != DuiSig_end) {
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
    const DUI_MSGMAP_ENTRY* lpEntry = NULL;
    const DUI_MSGMAP* pMessageMap = NULL;

    for(pMessageMap = GetMessageMap(); pMessageMap!=NULL; pMessageMap = pMessageMap->pBaseMap) {
        ASSERT(pMessageMap != pMessageMap->pBaseMap);
        if ((lpEntry = DuiFindMessageEntry(pMessageMap->lpEntries,msg)) != NULL)
        {
            goto LDispatch;
        }
    }
    return false;

LDispatch:
    union DuiMessageMapFunctions mmf;
    mmf.pfn = lpEntry->pfn;

    bool bRet = false;
    int nSig;
    nSig = lpEntry->nSig;
    switch (nSig)
    {
    default:
        ASSERT(FALSE);
        break;
    case DuiSig_lwl:
        (this->*mmf.pfn_Notify_lwl)(msg.wParam,msg.lParam);
        bRet = true;
        break;
    case DuiSig_vn:
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
            if (LPCTSTR key = m_VirtualWndMap.GetAt(i)) {
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
_oldWndProc(::DefWindowProc),
_bSubclassed(false)
{
}

Window::~Window()
{
    if (_hWnd != 0) {
        ActiveWindows::getInstance()->remove(_hWnd);
    }
}

HWND Window::GetHWND() const throw()
{ 
    return _hWnd; 
}

UINT Window::GetClassStyle() const throw()
{
    return 0;
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

HWND Window::Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x, int y, int cx, int cy, HMENU hMenu)
{
    if (GetSuperClassName() != NULL && !RegisterSuperclass()) {
        return NULL;
    }

    if (GetSuperClassName() == NULL && !RegisterWindowClass()) {
        return NULL;
    }

    _hWnd = ::CreateWindowEx(dwExStyle, GetWindowClassName(), pstrName, dwStyle, x, y, cx, cy, hwndParent, hMenu, CPaintManagerUI::GetInstance(), this);
    ASSERT(_hWnd!=NULL);

    if (_hWnd != 0) {
        ActiveWindows::getInstance()->add(_hWnd);
    }
    return _hWnd;
}

HWND Window::Subclass(HWND hWnd)
{
    ASSERT(::IsWindow(hWnd));
    ASSERT(_hWnd==NULL);
    _oldWndProc = SubclassWindow(hWnd, __WndProc);
    if (_oldWndProc == NULL) {
        return NULL;
    }
    _bSubclassed = true;
    _hWnd = hWnd;
    ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(this));
    return _hWnd;
}

void Window::Unsubclass()
{
    ASSERT(::IsWindow(_hWnd));
    if (!::IsWindow(_hWnd)) {
        return;
    }
    if (!_bSubclassed) {
        return;
    }
    SubclassWindow(_hWnd, _oldWndProc);
    _oldWndProc = ::DefWindowProc;
    _bSubclassed = false;
}

void Window::ShowWindow(bool bShow /*= true*/, bool bTakeFocus /*= false*/)
{
    ASSERT(::IsWindow(_hWnd));
    if (!::IsWindow(_hWnd)) {
        return;
    }
    ::ShowWindow(_hWnd, bShow ? (bTakeFocus ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE) : SW_HIDE);
}

UINT Window::ShowModal()
{
    ASSERT(::IsWindow(_hWnd));
    UINT nRet = 0;
    HWND hWndParent = GetWindowOwner(_hWnd);
    ::ShowWindow(_hWnd, SW_SHOWNORMAL);
    ::EnableWindow(hWndParent, FALSE);
    MSG msg = { 0 };
    while (::IsWindow(_hWnd) && ::GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_CLOSE && msg.hwnd == _hWnd) {
            nRet = msg.wParam;
            ::EnableWindow(hWndParent, TRUE);
            ::SetFocus(hWndParent);
        }
        if (!CPaintManagerUI::TranslateMessage(&msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT) {
            break;
        }
    }
    ::EnableWindow(hWndParent, TRUE);
    ::SetFocus(hWndParent);
    if (msg.message == WM_QUIT) {
        ::PostQuitMessage(msg.wParam);
    }
    return nRet;
}

void Window::Close(UINT nRet)
{
    ASSERT(::IsWindow(_hWnd));
    if (!::IsWindow(_hWnd)) {
        return;
    }
    PostMessage(WM_CLOSE, (WPARAM)nRet, 0L);
}

void Window::CenterWindow()
{
    ASSERT(::IsWindow(_hWnd));
    ASSERT((GetWindowStyle(_hWnd)&WS_CHILD)==0);
    RECT rcDlg = { 0 };
    ::GetWindowRect(_hWnd, &rcDlg);
    RECT rcArea = { 0 };
    RECT rcCenter = { 0 };
    HWND hWndParent = ::GetParent(_hWnd);
    HWND hWndCenter = ::GetWindowOwner(_hWnd);
    ::SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);
    if (hWndCenter == 0) {
        rcCenter = rcArea; 
    }
    else {
        ::GetWindowRect(hWndCenter, &rcCenter);
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
    ::SetWindowPos(_hWnd, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::SetIcon(UINT nRes)
{
    HICON hIcon = (HICON)::LoadImage(CPaintManagerUI::GetInstance(), MAKEINTRESOURCE(nRes), IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    ASSERT(hIcon);
    ::SendMessage(_hWnd, WM_SETICON, (WPARAM) TRUE, (LPARAM) hIcon);
    hIcon = (HICON)::LoadImage(CPaintManagerUI::GetInstance(), MAKEINTRESOURCE(nRes), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    ASSERT(hIcon);
    ::SendMessage(_hWnd, WM_SETICON, (WPARAM) FALSE, (LPARAM) hIcon);
}

bool Window::RegisterWindowClass()
{
    WNDCLASS wc = { 0 };
    wc.style = GetClassStyle();
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = NULL;
    wc.lpfnWndProc = Window::__WndProc;
    wc.hInstance = CPaintManagerUI::GetInstance();
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = GetWindowClassName();
    ATOM ret = ::RegisterClass(&wc);
    ASSERT(ret!=NULL || ::GetLastError()==ERROR_CLASS_ALREADY_EXISTS);
    return ret != NULL || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool Window::RegisterSuperclass()
{
    // Get the class information from an existing
    // window so we can subclass it later on...
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!::GetClassInfoEx(NULL, GetSuperClassName(), &wc)) {
        if (!::GetClassInfoEx(CPaintManagerUI::GetInstance(), GetSuperClassName(), &wc)) {
            ASSERT(!"Unable to locate window class");
            return NULL;
        }
    }
    _oldWndProc = wc.lpfnWndProc;
    wc.lpfnWndProc = Window::__ControlProc;
    wc.hInstance = CPaintManagerUI::GetInstance();
    wc.lpszClassName = GetWindowClassName();
    ATOM ret = ::RegisterClassEx(&wc);
    ASSERT(ret!=NULL || ::GetLastError()==ERROR_CLASS_ALREADY_EXISTS);
    return ret != NULL || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

LRESULT CALLBACK Window::__WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* pThis = NULL;
    if (uMsg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<Window*>(lpcs->lpCreateParams);
        pThis->_hWnd = hWnd;
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pThis));
    } 
    else {
        pThis = reinterpret_cast<Window*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (uMsg == WM_NCDESTROY && pThis != NULL) {
            LRESULT lRes = ::CallWindowProc(pThis->_oldWndProc, hWnd, uMsg, wParam, lParam);
            ::SetWindowLongPtr(pThis->_hWnd, GWLP_USERDATA, 0L);
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
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK Window::__ControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* pThis = NULL;
    if( uMsg == WM_NCCREATE ) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<Window*>(lpcs->lpCreateParams);
        ::SetProp(hWnd, _T("WndX"), (HANDLE)pThis);
        pThis->_hWnd = hWnd;
    } 
    else {
        pThis = reinterpret_cast<Window*>(::GetProp(hWnd, _T("WndX")));
        if (uMsg == WM_NCDESTROY && pThis != 0) {
            LRESULT lRes = ::CallWindowProc(pThis->_oldWndProc, hWnd, uMsg, wParam, lParam);
            if( pThis->_bSubclassed ) pThis->Unsubclass();
            ::SetProp(hWnd, _T("WndX"), 0);
            pThis->_hWnd = 0;
            pThis->OnFinalMessage(hWnd);
            return lRes;
        }
    }
    if (pThis != NULL) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    } 
    else {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

LRESULT Window::SendMessage(UINT uMsg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
    ASSERT(::IsWindow(_hWnd));
    return ::SendMessage(_hWnd, uMsg, wParam, lParam);
} 

LRESULT Window::PostMessage(UINT uMsg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
    ASSERT(::IsWindow(_hWnd));
    return ::PostMessage(_hWnd, uMsg, wParam, lParam);
}

void Window::ResizeClient(int cx /*= -1*/, int cy /*= -1*/)
{
    ASSERT(::IsWindow(_hWnd));
    RECT rc = { 0 };
    if (!::GetClientRect(_hWnd, &rc)) {
        return;
    }
    if (cx != -1) {
        rc.right = cx;
    }
    if (cy != -1) {
        rc.bottom = cy;
    }
    if (!::AdjustWindowRectEx(&rc, GetWindowStyle(_hWnd), (!(GetWindowStyle(_hWnd) & WS_CHILD) && (::GetMenu(_hWnd) != NULL)), GetWindowExStyle(_hWnd))) {
        return;
    }
    ::SetWindowPos(_hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return ::CallWindowProc(_oldWndProc, _hWnd, uMsg, wParam, lParam);
}

void Window::OnFinalMessage(HWND /*hWnd*/)
{
}

zgui_ImplementSingleton(ActiveWindows)

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

} // namespace zgui
