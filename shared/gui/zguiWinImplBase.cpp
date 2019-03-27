#include "zgui.h"

#define USE(FEATURE) (defined USE_##FEATURE  && USE_##FEATURE)
#define ENABLE(FEATURE) (defined ENABLE_##FEATURE  && ENABLE_##FEATURE)

namespace zgui
{

LPBYTE WindowImplBase::_lpResourceZIPBuffer=NULL;

ZGUI_BEGIN_MESSAGE_MAP(WindowImplBase,CNotifyPump)
	ZGUI_ON_MSGTYPE(ZGUI_MSGTYPE_CLICK, OnClick)
ZGUI_END_MESSAGE_MAP()

void WindowImplBase::OnFinalMessage(HWND hWnd)
{
	_paintManager.RemovePreMessageFilter(this);
	_paintManager.RemoveNotifier(this);
	_paintManager.ReapObjects(_paintManager.GetRoot());
}

LRESULT WindowImplBase::ResponseDefaultKeyEvent(WPARAM wParam)
{
	if (wParam == VK_RETURN)
	{
		return FALSE;
	}
	else if (wParam == VK_ESCAPE)
	{
		Close();
		return TRUE;
	}

	return FALSE;
}

UINT WindowImplBase::GetClassStyle() const
{
	return CS_DBLCLKS;
}

Control* WindowImplBase::createControl(const String& className)
{
	return NULL;
}

LRESULT WindowImplBase::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, bool& /*bHandled*/)
{
	if (uMsg == WM_KEYDOWN)
	{
		switch (wParam)
		{
		case VK_RETURN:
		case VK_ESCAPE:
			return ResponseDefaultKeyEvent(wParam);
		default:
			break;
		}
	}
	return FALSE;
}

LRESULT WindowImplBase::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

#if defined(WIN32)
LRESULT WindowImplBase::OnNcActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    if (fn_IsIconic(*this)) {
        bHandled = FALSE;
    }
	return (wParam == 0) ? TRUE : FALSE;
}

LRESULT WindowImplBase::OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LPRECT pRect = 0;

	if (wParam == TRUE) {
		LPNCCALCSIZE_PARAMS pParam = (LPNCCALCSIZE_PARAMS)lParam;
		pRect = &pParam->rgrc[0];
	}
	else {
		pRect=(LPRECT)lParam;
	}

	if (fn_IsZoomed(_hWnd)) {	// 最大化时，计算当前显示器最适合宽高度
		MONITORINFO oMonitor = {};
		oMonitor.cbSize = sizeof(oMonitor);
		fn_GetMonitorInfoW(fn_MonitorFromWindow(*this, MONITOR_DEFAULTTONEAREST), &oMonitor);
		Rect rcWork = oMonitor.rcWork;
		Rect rcMonitor = oMonitor.rcMonitor;
		rcWork.offsetWith(-oMonitor.rcMonitor.left, -oMonitor.rcMonitor.top);

		pRect->right = pRect->left + rcWork.getWidth();
		pRect->bottom = pRect->top + rcWork.getHeight();
		return WVR_REDRAW;
	}

	return 0;
}

LRESULT WindowImplBase::OnNcPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	return 0;
}

LRESULT WindowImplBase::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
	fn_ScreenToClient(*this, &pt);

	RECT rcClient;
	fn_GetClientRect(*this, &rcClient);

	if (!fn_IsZoomed(*this)) {
		RECT rcSizeBox = _paintManager.GetSizeBox();
		if (pt.y < rcClient.top + rcSizeBox.top) {
			if (pt.x < rcClient.left + rcSizeBox.left) {
				return HTTOPLEFT;
			}
			if (pt.x > rcClient.right - rcSizeBox.right) {
				return HTTOPRIGHT;
			}
			return HTTOP;
		}
		else if (pt.y > rcClient.bottom - rcSizeBox.bottom) {
			if (pt.x < rcClient.left + rcSizeBox.left) {
				return HTBOTTOMLEFT;
			}
			if (pt.x > rcClient.right - rcSizeBox.right) {
				return HTBOTTOMRIGHT;
			}
			return HTBOTTOM;
		}

		if (pt.x < rcClient.left + rcSizeBox.left) {
			return HTLEFT;
		}
		if (pt.x > rcClient.right - rcSizeBox.right) {
			return HTRIGHT;
		}
	}

	RECT rcCaption = _paintManager.GetCaptionRect();
	if (pt.x >= rcClient.left + rcCaption.left && pt.x < rcClient.right - rcCaption.right && pt.y >= rcCaption.top && pt.y < rcCaption.bottom) {
		Control* pControl = static_cast<Control*>(_paintManager.FindControl(pt));
		if (pControl && pControl->getClass().compareIgnoreCase(ZGUI_BUTTON) != 0 && pControl->getClass().compareIgnoreCase(ZGUI_TEXT) != 0) {
			return HTCAPTION;
		}
	}

	return HTCLIENT;
}

LRESULT WindowImplBase::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LPMINMAXINFO lpMMI = (LPMINMAXINFO) lParam;
	MONITORINFO oMonitor = {};
	oMonitor.cbSize = sizeof(oMonitor);
	fn_GetMonitorInfoW(fn_MonitorFromWindow(*this, MONITOR_DEFAULTTONEAREST), &oMonitor);
	Rect rcWork = oMonitor.rcWork;
	Rect rcMonitor = oMonitor.rcMonitor;
	rcWork.offsetWith(-oMonitor.rcMonitor.left, -oMonitor.rcMonitor.top);

	lpMMI->ptMaxPosition.x	= rcWork.left;
	lpMMI->ptMaxPosition.y	= rcWork.top;
	
	lpMMI->ptMaxTrackSize.x =rcWork.getWidth();
	lpMMI->ptMaxTrackSize.y =rcWork.getHeight();

	lpMMI->ptMinTrackSize.x = _paintManager.GetMinInfo().cx;
	lpMMI->ptMinTrackSize.y = _paintManager.GetMinInfo().cy;

	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnMouseWheel(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}
#endif

LRESULT WindowImplBase::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SIZE szRoundCorner = _paintManager.GetRoundCorner();
#if defined(WIN32)
	if (!fn_IsIconic(*this) && (szRoundCorner.cx != 0 || szRoundCorner.cy != 0)) {
		Rect rcWnd;
		fn_GetWindowRect(*this, &rcWnd);
		rcWnd.offsetWith(-rcWnd.left, -rcWnd.top);
		rcWnd.right++; rcWnd.bottom++;
		HRGN hRgn = fn_CreateRoundRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom, szRoundCorner.cx, szRoundCorner.cy);
        fn_SetWindowRgn(*this, hRgn, TRUE);
		fn_DeleteObject(hRgn);
	}
#endif
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == SC_CLOSE)
	{
		bHandled = TRUE;
		sendMessage(WM_CLOSE);
		return 0;
	}
#if defined(WIN32)
    BOOL bZoomed = fn_IsZoomed(*this);
	LRESULT lRes = Window::HandleMessage(uMsg, wParam, lParam);
	if (fn_IsZoomed(*this) != bZoomed) {
	}
#else
	LRESULT lRes = Window::HandleMessage(uMsg, wParam, lParam);
#endif
	return lRes;
}

LRESULT WindowImplBase::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LONG styleValue = fn_GetWindowLongW(*this, GWL_STYLE);
	styleValue &= ~WS_CAPTION;
	fn_SetWindowLongW(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	RECT rcClient;
	fn_GetClientRect(*this, &rcClient);
	fn_SetWindowPos(*this, NULL, rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, SWP_FRAMECHANGED);

	_paintManager.Init(_hWnd);
	_paintManager.AddPreMessageFilter(this);

	GuiBuilder builder;
	Control* pRoot = builder.create(GetSkinFile(), this, &_paintManager);
	zgui_assert(pRoot);
	if (pRoot==NULL) {
		fn_MessageBoxW(NULL, L"Some error", L"GUI", MB_OK|MB_ICONERROR);
		fn_ExitProcess(1);
		return 0;
	}
	_paintManager.attachRootControl(pRoot);
	_paintManager.AddNotifier(this);
	_paintManager.SetBackgroundTransparent(TRUE);

	InitWindow();
	return 0;
}

LRESULT WindowImplBase::OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT WindowImplBase::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
	switch (uMsg)
	{
	case WM_CREATE:			lRes = OnCreate(uMsg, wParam, lParam, bHandled); break;
	case WM_CLOSE:			lRes = OnClose(uMsg, wParam, lParam, bHandled); break;
	case WM_DESTROY:		lRes = OnDestroy(uMsg, wParam, lParam, bHandled); break;
#if defined(WIN32)
	case WM_NCACTIVATE:		lRes = OnNcActivate(uMsg, wParam, lParam, bHandled); break;
	case WM_NCCALCSIZE:		lRes = OnNcCalcSize(uMsg, wParam, lParam, bHandled); break;
	case WM_NCPAINT:		lRes = OnNcPaint(uMsg, wParam, lParam, bHandled); break;
	case WM_NCHITTEST:		lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
	case WM_GETMINMAXINFO:	lRes = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled); break;
	case WM_MOUSEWHEEL:		lRes = OnMouseWheel(uMsg, wParam, lParam, bHandled); break;
#endif
	case WM_SIZE:			lRes = OnSize(uMsg, wParam, lParam, bHandled); break;
	case WM_CHAR:		lRes = OnChar(uMsg, wParam, lParam, bHandled); break;
	case WM_SYSCOMMAND:		lRes = OnSysCommand(uMsg, wParam, lParam, bHandled); break;
	case WM_KEYDOWN:		lRes = OnKeyDown(uMsg, wParam, lParam, bHandled); break;
	case WM_KILLFOCUS:		lRes = OnKillFocus(uMsg, wParam, lParam, bHandled); break;
	case WM_SETFOCUS:		lRes = OnSetFocus(uMsg, wParam, lParam, bHandled); break;
	case WM_LBUTTONUP:		lRes = OnLButtonUp(uMsg, wParam, lParam, bHandled); break;
	case WM_LBUTTONDOWN:	lRes = OnLButtonDown(uMsg, wParam, lParam, bHandled); break;
	case WM_MOUSEMOVE:		lRes = OnMouseMove(uMsg, wParam, lParam, bHandled); break;
	case WM_MOUSEHOVER:	lRes = OnMouseHover(uMsg, wParam, lParam, bHandled); break;
	default:				bHandled = FALSE; break;
	}
	if (bHandled) return lRes;

	lRes = HandleCustomMessage(uMsg, wParam, lParam, bHandled);
	if (bHandled) return lRes;

	if (_paintManager.MessageHandler(uMsg, wParam, lParam, lRes))
		return lRes;
	return Window::HandleMessage(uMsg, wParam, lParam);
}

LRESULT WindowImplBase::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LONG WindowImplBase::GetStyle()
{
	LONG styleValue = fn_GetWindowLongW(*this, GWL_STYLE);
	styleValue &= ~WS_CAPTION;

	return styleValue;
}

void WindowImplBase::OnClick(TNotifyUI& msg)
{
	String sCtrlName = msg.pSender->GetName();
	if (sCtrlName == "closebtn") {
		Close();
		return; 
	}
	else if (sCtrlName == "minbtn") { 
		sendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0); 
		return; 
	}
	else if (sCtrlName == "maxbtn") { 
		sendMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0); 
		return; 
	}
	else if (sCtrlName == "restorebtn") { 
		sendMessage(WM_SYSCOMMAND, SC_RESTORE, 0); 
		return; 
	}
	return;
}

void WindowImplBase::Notify(TNotifyUI& msg)
{
	return CNotifyPump::NotifyPump(msg);
}

}