#include "zgui.h"
#include <zmouse.h>

namespace zgui {

#define WM_EFFECTS WM_USER+1680

static UINT MapKeyState()
{
    UINT uState = 0;
    if( fn_GetKeyState(VK_CONTROL) < 0 ) uState |= MK_CONTROL;
    if( fn_GetKeyState(VK_RBUTTON) < 0 ) uState |= MK_LBUTTON;
    if( fn_GetKeyState(VK_LBUTTON) < 0 ) uState |= MK_RBUTTON;
    if( fn_GetKeyState(VK_SHIFT) < 0 ) uState |= MK_SHIFT;
    if( fn_GetKeyState(VK_MENU) < 0 ) uState |= MK_ALT;
    return uState;
}

typedef struct tagFINDTABINFO
{
    Control* pFocus;
    Control* pLast;
    bool bForward;
    bool bNextIsIt;
} FINDTABINFO;

typedef struct tagFINDSHORTCUT
{
    TCHAR ch;
    bool bPickNext;
} FINDSHORTCUT;


/////////////////////////////////////////////////////////////////////////////////////

HPEN m_hUpdateRectPen = 0;
HINSTANCE PaintManager::m_hInstance = 0;
String PaintManager::m_pStrDefaultFontName;//added by cddjr at 05/18/2012
short PaintManager::m_H = 180;
short PaintManager::m_S = 100;
short PaintManager::m_L = 100;
Array<PaintManager*> PaintManager::_preMessages;

static BOOL CALLBACK enumMonitorsProc(HMONITOR, HDC, LPRECT r, LPARAM userInfo)
{
    RECT* monitorRect = (RECT*)userInfo;
    __movsb((uint8_t*)monitorRect, (const uint8_t*)r, sizeof(RECT));
    return FALSE;
}

void TimerTickProc(LPARAM lParam)
{
	PaintManager* pPM = (PaintManager*)lParam;
	if (pPM) {
		pPM->HideTooltip();
	}
}

void PaintManager::HideTooltip()
{
	if (_hwndTooltip != NULL) {
		fn_SendMessageW(_hwndTooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&_tooltip);
		_tooltipTimerWnd.killTimer();
	}
}

PaintManager::PaintManager() :
_hwndPaint(0),
_hdcPaint(0),
_hdcOffscreen(0),
_hdcBackground(0),
_hbmpOffscreen(0),
_hbmpBackground(0),
_hwndTooltip(0),
m_bShowUpdateRect(false),
m_uTimerID(0x1000),
_pRootControl(0),
m_pFocus(0),
m_pEventHover(0),
m_pEventClick(0),
m_pEventKey(0),
m_bFirstLayout(true),
m_bFocusNeeded(false),
m_bUpdateNeeded(false),
m_bMouseTracking(false),
m_bMouseCapture(false),
m_bUsedVirtualWnd(false),
m_bOffscreenPaint(true),
m_bAlphaBackground(false),
_nOpacity(255),
m_pParentResourcePM(0)
{
    m_dwDefaultDisabledColor = 0xFFA7A6AA;
    m_dwDefaultFontColor = 0xFF000000;
    _dwDefaultLinkFontColor = 0xFF2118d5;
    _dwDefaultLinkHoverFontColor = 0xFF2118d5;
    m_dwDefaultSelectedBkColor = 0xFFBAE4FF;
    LOGFONTW lf;
    __stosb((uint8_t*)&lf, 0, sizeof(lf));
    fn_GetObjectW(fn_GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    lf.lfCharSet = DEFAULT_CHARSET;
	if (PaintManager::m_pStrDefaultFontName.length() > 0) {
        PaintManager::m_pStrDefaultFontName.copyToUTF16(lf.lfFaceName, sizeof(lf.lfFaceName) - 2);
	}
    HFONT hDefaultFont = fn_CreateFontIndirectW(&lf);
    m_DefaultFontInfo.hFont = hDefaultFont;
    m_DefaultFontInfo.sFontName = lf.lfFaceName;
    m_DefaultFontInfo.iSize = -lf.lfHeight;
    m_DefaultFontInfo.bBold = (lf.lfWeight >= FW_BOLD);
    m_DefaultFontInfo.bUnderline = (lf.lfUnderline == TRUE);
    m_DefaultFontInfo.bItalic = (lf.lfItalic == TRUE);
    __stosb((uint8_t*)&m_DefaultFontInfo.tm, 0, sizeof(m_DefaultFontInfo.tm));

    if( m_hUpdateRectPen == 0 ) {
        m_hUpdateRectPen = fn_CreatePen(PS_SOLID, 1, RGB(220, 0, 0));
       
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_TAB_CLASSES;
		fn_InitCommonControlsEx(&icex);
    }

    m_szMinWindow.cx = 0;
    m_szMinWindow.cy = 0;
    m_szMaxWindow.cx = 0;
    m_szMaxWindow.cy = 0;
    m_szInitWindowSize.cx = 0;
    m_szInitWindowSize.cy = 0;
    m_szRoundCorner.cx = m_szRoundCorner.cy = 0;
    __stosb((uint8_t*)&m_rcSizeBox, 0, sizeof(m_rcSizeBox));
    __stosb((uint8_t*)&m_rcCaption, 0, sizeof(m_rcCaption));
    m_ptLastMousePos.x = m_ptLastMousePos.y = -1;

    fn_EnumDisplayMonitors(0, 0, &enumMonitorsProc, (LPARAM)&_monitorRect);
}

PaintManager::~PaintManager()
{
    int i;
    // Delete the control-tree structures
    for (i = 0; i < _delayedCleanup.size(); ++i) {
        delete _delayedCleanup.getUnchecked(i);
    }
    for (i = 0; i < _asyncNotify.size(); ++i) {
        delete _asyncNotify.getUnchecked(i);
    }

    m_mNameHash.Resize(0);
    delete _pRootControl;

    fn_DeleteObject(m_DefaultFontInfo.hFont);
    RemoveAllFonts();
    removeAllImages();
    RemoveAllDefaultAttributeList();
	removeAllStyles();
    RemoveAllOptionGroups();
    RemoveAllTimers();

    // Reset other parts...
    if (_hwndTooltip != 0) {
        fn_DestroyWindow(_hwndTooltip);
    }
    if (_hdcOffscreen != 0) {
        fn_DeleteDC(_hdcOffscreen);
    }
    if (_hdcBackground != 0) {
        fn_DeleteDC(_hdcBackground);
    }
    if (_hbmpOffscreen != 0) {
        fn_DeleteObject(_hbmpOffscreen);
    }
    if (_hbmpBackground != 0) {
        fn_DeleteObject(_hbmpBackground);
    }
    if (_hdcPaint != 0) {
        fn_ReleaseDC(_hwndPaint, _hdcPaint);
    }
    _preMessages.remove(_preMessages.indexOf(this));
}

void PaintManager::Init(HWND hWnd)
{
    zgui_assert(fn_IsWindow(hWnd));
    // Remember the window context we came from
    _hwndPaint = hWnd;
    _hdcPaint = fn_GetDC(hWnd);
    // We'll want to filter messages globally too
    _preMessages.add(this);
}

HINSTANCE PaintManager::GetInstance()
{
    return m_hInstance;
}

String PaintManager::GetInstancePath()
{
    wchar_t tszModule[MAX_PATH + 1];
    String sInstancePath;

    if (m_hInstance == 0) {
        return String::empty;
    }
    
    if (fn_GetModuleFileNameW(m_hInstance, tszModule, MAX_PATH) > 0) {
        sInstancePath = tszModule;
        int pos = sInstancePath.lastIndexOfChar('\\');
        if (pos >= 0) {
            sInstancePath = sInstancePath.substring(0, pos + 1);
        }
    }
    return sInstancePath;
}

String PaintManager::GetCurrentPath()
{
    wchar_t tszModule[MAX_PATH + 1];
    __stosb((uint8_t*)tszModule, 0, sizeof(tszModule));
    fn_GetCurrentDirectoryW(MAX_PATH, tszModule);
    return tszModule;
}

void PaintManager::SetInstance(HINSTANCE hInst)
{
    m_hInstance = hInst;
}

void PaintManager::SetCurrentPath(LPCWSTR pStrPath)
{
    fn_SetCurrentDirectoryW(pStrPath);
}

void PaintManager::GetHSL(short* H, short* S, short* L)
{
    *H = m_H;
    *S = m_S;
    *L = m_L;
}

void PaintManager::SetHSL(bool bUseHSL, short H, short S, short L)
{
    if( H == m_H && S == m_S && L == m_L ) return;
    m_H = CLAMP(H, 0, 360);
    m_S = CLAMP(S, 0, 200);
    m_L = CLAMP(L, 0, 200);
    for (int i = 0; i < _preMessages.size(); ++i) {
        PaintManager* pManager = _preMessages.getUnchecked(i);
		if (pManager != 0 && pManager->GetRoot() != 0) {
			pManager->GetRoot()->Invalidate();
		}
    }
}

void PaintManager::ReloadSkin()
{
    for (int i = 0; i < _preMessages.size(); ++i) {
        PaintManager* pManager = _preMessages.getUnchecked(i);
        pManager->reloadAllImages();
    }
}

LPRECT PaintManager::getMonitorArea()
{
    return (LPRECT)&_monitorRect;
}

HWND PaintManager::GetPaintWindow() const
{
    return _hwndPaint;
}

HWND PaintManager::GetTooltipWindow() const
{
    return _hwndTooltip;
}

HDC PaintManager::GetPaintDC() const
{
    return _hdcPaint;
}

POINT PaintManager::GetMousePos() const
{
    return m_ptLastMousePos;
}

SIZE PaintManager::GetClientSize() const
{
    RECT rcClient;
    __stosb((uint8_t*)&rcClient, 0, sizeof(rcClient));
    fn_GetClientRect(_hwndPaint, &rcClient);
    return Size(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
}

SIZE PaintManager::GetInitSize()
{
    return m_szInitWindowSize;
}

void PaintManager::SetInitSize(int cx, int cy)
{
    m_szInitWindowSize.cx = cx;
    m_szInitWindowSize.cy = cy;
    if (_pRootControl == 0 && _hwndPaint != 0) {
        fn_SetWindowPos(_hwndPaint, 0, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
    }
}

RECT& PaintManager::GetSizeBox()
{
    return m_rcSizeBox;
}

void PaintManager::SetSizeBox(RECT& rcSizeBox)
{
    m_rcSizeBox = rcSizeBox;
}

RECT& PaintManager::GetCaptionRect()
{
    return m_rcCaption;
}

void PaintManager::SetCaptionRect(RECT& rcCaption)
{
    m_rcCaption = rcCaption;
}

SIZE PaintManager::GetRoundCorner() const
{
    return m_szRoundCorner;
}

void PaintManager::SetRoundCorner(int cx, int cy)
{
    m_szRoundCorner.cx = cx;
    m_szRoundCorner.cy = cy;
}

SIZE PaintManager::GetMinInfo() const
{
	return m_szMinWindow;
}

void PaintManager::SetMinInfo(int cx, int cy)
{
    zgui_assert(cx>=0 && cy>=0);
    m_szMinWindow.cx = cx;
    m_szMinWindow.cy = cy;
}

SIZE PaintManager::GetMaxInfo() const
{
    return m_szMaxWindow;
}

void PaintManager::SetMaxInfo(int cx, int cy)
{
    zgui_assert(cx>=0 && cy>=0);
    m_szMaxWindow.cx = cx;
    m_szMaxWindow.cy = cy;
}

int PaintManager::GetTransparent() const
{
	return _nOpacity;
}
void PaintManager::SetTransparent(int nOpacity)
{
	if (nOpacity < 0)
		_nOpacity = 0;
	else if (nOpacity>255)
		_nOpacity = 255;
	else
		_nOpacity = nOpacity;
    if (_hwndPaint != 0) {
        DWORD dwStyle = fn_GetWindowLongW(_hwndPaint, GWL_EXSTYLE);
        DWORD dwNewStyle = dwStyle;
        if( nOpacity >= 0 && nOpacity < 256 ) dwNewStyle |= WS_EX_LAYERED;
        else dwNewStyle &= ~WS_EX_LAYERED;
        if (dwStyle != dwNewStyle) {
            fn_SetWindowLongW(_hwndPaint, GWL_EXSTYLE, dwNewStyle);
        }
        fn_SetLayeredWindowAttributes(_hwndPaint, 0, nOpacity, LWA_ALPHA);
    }
}

void PaintManager::SetBackgroundTransparent(bool bTrans)
{
    m_bAlphaBackground = bTrans;
}

bool PaintManager::IsShowUpdateRect() const
{
	return m_bShowUpdateRect;
}

void PaintManager::SetShowUpdateRect(bool show)
{
    m_bShowUpdateRect = show;
}

bool PaintManager::PreMessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& /*lRes*/)
{
    for (int i = 0; i < _preMessageFilters.size(); ++i) {
        bool bHandled = false;
        LRESULT lResult = _preMessageFilters.getUnchecked(i)->MessageHandler(uMsg, wParam, lParam, bHandled);
        if (bHandled) {
            return true;
        }
    }

    switch (uMsg) {
        case WM_KEYDOWN:
            {
               // Tabbing between controls
               if (wParam == VK_TAB) {
#ifdef ZGUI_USE_RICHEDIT
                   if (m_pFocus && m_pFocus->IsVisible() && m_pFocus->IsEnabled() && _tcsstr(m_pFocus->getClass(), _T("RichEditUI")) != 0) {
                       if (static_cast<CRichEditUI*>(m_pFocus)->IsWantTab()) {
                           return false;
                       }
                   }
#endif // ZGUI_USE_RICHEDIT
                   SetNextTabControl(fn_GetKeyState(VK_SHIFT) >= 0);
                   return true;
               }
            }
            break;
        case WM_SYSCHAR:
            {
               // Handle ALT-shortcut key-combinations
               FINDSHORTCUT fs;
               __stosb((uint8_t*)&fs, 0, sizeof(fs));
               fs.ch = CharacterFunctions::toUpperCase((int)wParam);
               Control* pControl = _pRootControl->FindControl(__FindControlFromShortcut, &fs, UIFIND_ENABLED | UIFIND_ME_FIRST | UIFIND_TOP_FIRST);
               if( pControl != 0 ) {
                   pControl->SetFocus();
                   pControl->Activate();
                   return true;
               }
            }
            break;
        case WM_SYSKEYDOWN:
            {
               if( m_pFocus != 0 ) {
                   TEventUI event;
                   __stosb((uint8_t*)&event, 0, sizeof(event));
                   event.Type = UIEVENT_SYSKEY;
                   event.chKey = (TCHAR)wParam;
                   event.ptMouse = m_ptLastMousePos;
                   event.wKeyState = MapKeyState();
                   event.dwTimestamp = fn_GetTickCount();
                   m_pFocus->Event(event);
               }
            }
            break;
    }
    return false;
}

bool PaintManager::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lRes)
{
//#ifdef _DEBUG
//    switch( uMsg ) {
//    case WM_NCPAINT:
//    case WM_NCHITTEST:
//    case WM_SETCURSOR:
//       break;
//    default:
//       TRACE(_T("MSG: %-20s (%08ld)"), TRACEMSG(uMsg), fn_GetTickCount());
//    }
//#endif
    // Not ready yet?
    if (_hwndPaint == 0) {
        return false;
    }
    
    TNotifyUI* pMsg = 0;
    while (pMsg = _asyncNotify[0]) {
        _asyncNotify.remove(0);
        if (pMsg->pSender != 0) {
            if (pMsg->pSender->OnNotify) {
                pMsg->pSender->OnNotify(pMsg);
            }
        }
        for (int j = 0; j < _notifiers.size(); ++j) {
            _notifiers.getUnchecked(j)->Notify(*pMsg);
        }
        delete pMsg;
    }
    
    // Cycle through listeners
    for (int i = 0; i < _messageFilters.size(); ++i) {
        bool bHandled = false;
        LRESULT lResult = _messageFilters.getUnchecked(i)->MessageHandler(uMsg, wParam, lParam, bHandled);
        if (bHandled) {
            lRes = lResult;
            return true;
        }
    }
    // Custom handling of events
    switch (uMsg) {
        case WM_APP + 1:
            {
                for (int i = 0; i < _delayedCleanup.size(); ++i) {
                    delete _delayedCleanup.getUnchecked(i);
                }
                _delayedCleanup.clear();
            }
            break;
        case WM_CLOSE:
            {
                // Make sure all matching "closing" events are sent
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.ptMouse = m_ptLastMousePos;
                event.dwTimestamp = fn_GetTickCount();
                if (m_pEventHover != 0) {
                    event.Type = UIEVENT_MOUSELEAVE;
                    event.pSender = m_pEventHover;
                    m_pEventHover->Event(event);
                }
                if (m_pEventClick != 0) {
                    event.Type = UIEVENT_BUTTONUP;
                    event.pSender = m_pEventClick;
                    m_pEventClick->Event(event);
                }

                SetFocus(0);

                // Hmmph, the usual Windows tricks to avoid
                // focus loss...
                HWND hwndParent = fn_GetWindow(_hwndPaint, GW_OWNER);
                if (hwndParent != 0) {
                    fn_SetFocus(hwndParent);
                }
            }
            break;
        case WM_ERASEBKGND:
            {
                // We'll do the painting here...
                lRes = 1;
            }
            return true;
#ifdef ZGUI_USE_ANIMATION
		case WM_EFFECTS: {
			// Render screen
			if (_anim.isAnimating()) {
				// 3D animation in progress
				//   3D动画  
				_anim.render();
				// Do a minimum paint loop  做一个最小的绘制循环
				// Keep the client area invalid so we generate lots of
				// WM_PAINT messages. Cross fingers that Windows doesn't
				// batch these somehow in the future.
				PAINTSTRUCT ps;
				__stosb((uint8_t*)&ps, 0, sizeof(ps));
				fn_BeginPaint(_hwndPaint, &ps);
				fn_EndPaint(_hwndPaint, &ps);
				fn_InvalidateRect(_hwndPaint, NULL, FALSE);
			}
			else if (_anim.isJobScheduled()) {
				// Animation system needs to be initialized
				//	动画系统需要初始化
				_anim.init(_hwndPaint);
				// A 3D animation was scheduled; allow the render engine to
				// capture the window content and repaint some other time
				//翻译(by 金山词霸)一个3d动画被准备;允许渲染引擎捕获窗口内容，并且适时重画

				if (!_anim.prepareAnimation(_hwndPaint)) {
					_anim.cancelJobs();
				}
				fn_InvalidateRect(_hwndPaint, NULL, TRUE);
			}
		}
			return true;
#endif
        case WM_PAINT:
            {
                // Should we paint?
                RECT rcPaint;
                __stosb((uint8_t*)&rcPaint, 0, sizeof(rcPaint));
                if (!fn_GetUpdateRect(_hwndPaint, &rcPaint, FALSE)) {
                    return true;
                }
                if (_pRootControl == 0) {
                    PAINTSTRUCT ps;
                    __stosb((uint8_t*)&ps, 0, sizeof(ps));
                    fn_BeginPaint(_hwndPaint, &ps);
                    fn_EndPaint(_hwndPaint, &ps);
                    return true;
                }            
                // Do we need to resize anything?
                // This is the time where we layout the controls on the form.
                // We delay this even from the WM_SIZE messages since resizing can be
                // a very expensize operation.
                if (m_bUpdateNeeded) {
                    m_bUpdateNeeded = false;
                    RECT rcClient;
                    __stosb((uint8_t*)&rcClient, 0, sizeof(rcClient));
                    fn_GetClientRect(_hwndPaint, &rcClient);
                    if (!fn_IsRectEmpty(&rcClient)) {
                        if (_pRootControl->IsUpdateNeeded()) {
                            _pRootControl->SetPos(rcClient);
                            if (_hdcOffscreen != 0) {
                                fn_DeleteDC(_hdcOffscreen);
                            }
                            if (_hdcBackground != 0) {
                                fn_DeleteDC(_hdcBackground);
                            }
                            if (_hbmpOffscreen != 0) {
                                fn_DeleteObject(_hbmpOffscreen);
                            }
                            if (_hbmpBackground != 0) {
                                fn_DeleteObject(_hbmpBackground);
                            }
                            _hdcOffscreen = 0;
                            _hdcBackground = 0;
                            _hbmpOffscreen = 0;
                            _hbmpBackground = 0;
                        }
                        else {
                            Control* pControl = 0;
                            while (pControl = _pRootControl->FindControl(__FindControlFromUpdate, 0, UIFIND_VISIBLE | UIFIND_ME_FIRST)) {
                                pControl->SetPos(pControl->GetPos());
                            }
                        }
                        // We'll want to notify the window when it is first initialized
                        // with the correct layout. The window form would take the time
                        // to submit swipes/animations.
                        if (m_bFirstLayout) {
                            m_bFirstLayout = false;
                            SendNotify(_pRootControl, ZGUI_MSGTYPE_WINDOWINIT, 0, 0, false);
                        }
                    }
                }
                // Set focus to first control?
                if (m_bFocusNeeded) {
                    SetNextTabControl();
                }

#ifdef ZGUI_USE_ANIMATION
				if (_anim.isAnimating() || _anim.isJobScheduled()) {
					fn_PostMessageW(_hwndPaint, WM_EFFECTS, NULL, NULL);
				}
				else {
#endif
					//
					// Render screen
					//
					// Prepare offscreen bitmap?
					if (m_bOffscreenPaint && _hbmpOffscreen == 0) {
						RECT rcClient;
						__stosb((uint8_t*)&rcClient, 0, sizeof(rcClient));
						fn_GetClientRect(_hwndPaint, &rcClient);
						_hdcOffscreen = fn_CreateCompatibleDC(_hdcPaint);
						_hbmpOffscreen = fn_CreateCompatibleBitmap(_hdcPaint, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
						zgui_assert(_hdcOffscreen);
						zgui_assert(_hbmpOffscreen);
					}
					// Begin Windows paint
					PAINTSTRUCT ps;
					__stosb((uint8_t*)&ps, 0, sizeof(ps));
					fn_BeginPaint(_hwndPaint, &ps);
					if (m_bOffscreenPaint) {
						HBITMAP hOldBitmap = (HBITMAP)fn_SelectObject(_hdcOffscreen, _hbmpOffscreen);
						int iSaveDC = fn_SaveDC(_hdcOffscreen);
						if (m_bAlphaBackground) {
							if (_hbmpBackground == 0) {
								RECT rcClient;
								__stosb((uint8_t*)&rcClient, 0, sizeof(rcClient));
								fn_GetClientRect(_hwndPaint, &rcClient);
								_hdcBackground = fn_CreateCompatibleDC(_hdcPaint);;
								_hbmpBackground = fn_CreateCompatibleBitmap(_hdcPaint, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
								zgui_assert(_hdcBackground);
								zgui_assert(_hbmpBackground);
								fn_SelectObject(_hdcBackground, _hbmpBackground);
								fn_BitBlt(_hdcBackground, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
									ps.rcPaint.bottom - ps.rcPaint.top, ps.hdc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
							}
							else {
								fn_SelectObject(_hdcBackground, _hbmpBackground);
							}
							fn_BitBlt(_hdcOffscreen, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
								ps.rcPaint.bottom - ps.rcPaint.top, _hdcBackground, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
						}
						_pRootControl->DoPaint(_hdcOffscreen, ps.rcPaint);
						for (int i = 0; i < _postPaintControls.size(); ++i) {
							Control* pPostPaintControl = _postPaintControls.getUnchecked(i);
							pPostPaintControl->DoPostPaint(_hdcOffscreen, ps.rcPaint);
						}
						fn_RestoreDC(_hdcOffscreen, iSaveDC);
						fn_BitBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
							ps.rcPaint.bottom - ps.rcPaint.top, _hdcOffscreen, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
						fn_SelectObject(_hdcOffscreen, hOldBitmap);

						if (m_bShowUpdateRect) {
							HPEN hOldPen = (HPEN)fn_SelectObject(ps.hdc, m_hUpdateRectPen);
							fn_SelectObject(ps.hdc, fn_GetStockObject(HOLLOW_BRUSH));
							fn_Rectangle(ps.hdc, rcPaint.left, rcPaint.top, rcPaint.right, rcPaint.bottom);
							fn_SelectObject(ps.hdc, hOldPen);
						}
					}
					else {
						// A standard paint job
						int iSaveDC = fn_SaveDC(ps.hdc);
						_pRootControl->DoPaint(ps.hdc, ps.rcPaint);
						fn_RestoreDC(ps.hdc, iSaveDC);
					}
					// All Done!
					fn_EndPaint(_hwndPaint, &ps);
#ifdef ZGUI_USE_ANIMATION
				}
#endif // ZGUI_USE_ANIMATION
            }
            // If any of the painting requested a resize again, we'll need
            // to invalidate the entire window once more.
            if (m_bUpdateNeeded) {
                fn_InvalidateRect(_hwndPaint, 0, FALSE);
            }
            return true;
        case WM_PRINTCLIENT:
            {
                RECT rcClient;
                fn_GetClientRect(_hwndPaint, &rcClient);
                HDC hDC = (HDC) wParam;
                int save = fn_SaveDC(hDC);
                _pRootControl->DoPaint(hDC, rcClient);
                // Check for traversing children. The crux is that WM_PRINT will assume
                // that the DC is positioned at frame coordinates and will paint the child
                // control at the wrong position. We'll simulate the entire thing instead.
                if ((lParam & PRF_CHILDREN) != 0) {
                    HWND hWndChild = fn_GetWindow(_hwndPaint, GW_CHILD);
                    while (hWndChild != 0) {
                        RECT rcPos;
                        __stosb((uint8_t*)&rcPos, 0, sizeof(rcPos));
                        fn_GetWindowRect(hWndChild, &rcPos);
                        fn_MapWindowPoints(HWND_DESKTOP, _hwndPaint, reinterpret_cast<LPPOINT>(&rcPos), 2);
                        fn_SetWindowOrgEx(hDC, -rcPos.left, -rcPos.top, 0);
                        // NOTE: We use WM_PRINT here rather than the expected WM_PRINTCLIENT
                        //       since the latter will not print the nonclient correctly for
                        //       EDIT controls.
                        fn_SendMessageW(hWndChild, WM_PRINT, wParam, lParam | PRF_NONCLIENT);
                        hWndChild = fn_GetWindow(hWndChild, GW_HWNDNEXT);
                    }
                }
                fn_RestoreDC(hDC, save);
            }
            break;
        case WM_GETMINMAXINFO:
            {
                LPMINMAXINFO lpMMI = (LPMINMAXINFO) lParam;
				if (m_szMinWindow.cx > 0) {
					lpMMI->ptMinTrackSize.x = m_szMinWindow.cx;
				}
				if (m_szMinWindow.cy > 0) {
					lpMMI->ptMinTrackSize.y = m_szMinWindow.cy;
				}
				if (m_szMaxWindow.cx > 0) {
					lpMMI->ptMaxTrackSize.x = m_szMaxWindow.cx;
				}
				if (m_szMaxWindow.cy > 0) {
					lpMMI->ptMaxTrackSize.y = m_szMaxWindow.cy;
				}
            }
            break;
        case WM_SIZE:
            {
                if (m_pFocus != 0) {
                    TEventUI event;
                    __stosb((uint8_t*)&event, 0, sizeof(event));
                    event.Type = UIEVENT_WINDOWSIZE;
                    event.pSender = m_pFocus;
                    event.dwTimestamp = fn_GetTickCount();
                    m_pFocus->Event(event);
                }
#ifdef ZGUI_USE_ANIMATION
				if (_anim.isAnimating()) {
					_anim.cancelJobs();
				}
#endif
				if (_pRootControl != 0) {
					_pRootControl->NeedUpdate();
				}
            }
            return true;
        case WM_TIMER:
            {
                for (int i = 0; i < _timers.size(); ++i) {
                    const TIMERINFO* pTimer = _timers.getUnchecked(i);
                    if( pTimer->hWnd == _hwndPaint && pTimer->uWinTimer == LOWORD(wParam) && pTimer->bKilled == false) {
                        TEventUI event;
                        __stosb((uint8_t*)&event, 0, sizeof(event));
                        event.Type = UIEVENT_TIMER;
                        event.pSender = pTimer->pSender;
                        event.wParam = pTimer->nLocalID;
                        event.dwTimestamp = fn_GetTickCount();
                        pTimer->pSender->Event(event);
                        break;
                    }
                }
            }
            break;
        case WM_MOUSEHOVER:
            {
				//m_bMouseTracking = false;
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                Control* pHover = FindControl(pt);
                if (pHover == 0) {
					break;
				}
                // Generate mouse hover event
                if( m_pEventHover != 0 ) {
                    TEventUI event;
                    __stosb((uint8_t*)&event, 0, sizeof(event));
                    event.ptMouse = pt;
                    event.Type = UIEVENT_MOUSEHOVER;
                    event.pSender = m_pEventHover;
                    event.dwTimestamp = fn_GetTickCount();
                    m_pEventHover->Event(event);
                }
                // Create tooltip information
                String sToolTip = pHover->GetToolTip();
				if (!sToolTip.isEmpty()) {
					if (_hwndTooltip == 0) {
						_hwndTooltip = fn_CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, _hwndPaint, 0, m_hInstance, 0);

						__stosb((uint8_t*)&_tooltip, 0, sizeof(TOOLINFOW));
						_tooltip.cbSize = sizeof(TOOLINFOW);
						_tooltip.uFlags = TTF_IDISHWND;
						_tooltip.hwnd = _hwndPaint;
						_tooltip.uId = (UINT_PTR)_hwndPaint;
						_tooltip.hinst = m_hInstance;
						fn_SendMessageW(_hwndTooltip, TTM_ADDTOOLW, 0, (LPARAM)&_tooltip);
					}

					_tooltip.lpszText = const_cast<LPWSTR>((LPCWSTR)sToolTip.toWideCharPointer());
					_tooltip.rect = pHover->GetPos();
					fn_SendMessageW(_hwndTooltip, TTM_SETMAXTIPWIDTH, 0, pHover->GetToolTipWidth());
					fn_SendMessageW(_hwndTooltip, TTM_UPDATETIPTEXT, 0, (LPARAM)&_tooltip);
					fn_SendMessageW(_hwndTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&_tooltip);

					_tooltipTimerWnd.setTimer(7000, TimerTickProc, (LPARAM)this);
				}
            }
            return true;
        case WM_MOUSELEAVE:
            {
				_tooltipTimerWnd.killTimer();
                if (m_bMouseTracking) {
                    fn_SendMessageW(_hwndPaint, WM_MOUSEMOVE, 0, (LPARAM)-1);
                }
            }
            break;
        case WM_MOUSEMOVE:
            {
                // Generate the appropriate mouse messages
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                m_ptLastMousePos = pt;
				Control* pNewHover = FindControl(pt);
				if (pNewHover != 0 && pNewHover->getManager() != this) {
					break;
				}
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.ptMouse = pt;
                event.dwTimestamp = fn_GetTickCount();
                if (pNewHover != m_pEventHover && m_pEventHover != 0) {
                    event.Type = UIEVENT_MOUSELEAVE;
                    event.pSender = m_pEventHover;
                    m_pEventHover->Event(event);
                    m_pEventHover = 0;
                    if (_hwndTooltip != 0) {
                        fn_SendMessageW(_hwndTooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM) &_tooltip);
                    }
					m_bMouseTracking = false;
                }
                if( pNewHover != m_pEventHover && pNewHover != 0 ) {
                    event.Type = UIEVENT_MOUSEENTER;
                    event.pSender = pNewHover;
                    pNewHover->Event(event);
                    m_pEventHover = pNewHover;
                }
                if( m_pEventClick != 0 ) {
                    event.Type = UIEVENT_MOUSEMOVE;
                    event.pSender = m_pEventClick;
                    m_pEventClick->Event(event);
                }
                else if( pNewHover != 0 ) {
                    event.Type = UIEVENT_MOUSEMOVE;
                    event.pSender = pNewHover;
                    pNewHover->Event(event);
                }

				// Start tracking this entire window again...
				if (!m_bMouseTracking/* && !m_IsDoDragDroping*/) {
                    TRACKMOUSEEVENT tme;
                    __stosb((uint8_t*)&tme, 0, sizeof(tme));
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_HOVER | TME_LEAVE;
                    tme.hwndTrack = _hwndPaint;
                    tme.dwHoverTime = _hwndTooltip == 0 ? 400UL : (DWORD)fn_SendMessageW(_hwndTooltip, TTM_GETDELAYTIME, TTDT_INITIAL, 0L);
                    fn__TrackMouseEvent(&tme);
                    m_bMouseTracking = true;
                }
            }
            break;
        case WM_LBUTTONDOWN:
            {
                // We alway set focus back to our app (this helps
                // when Win32 child windows are placed on the dialog
                // and we need to remove them on focus change).
                fn_SetFocus(_hwndPaint);
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                m_ptLastMousePos = pt;
                Control* pControl = FindControl(pt);
				if (pControl == 0 || pControl->getManager() != this) {
					break;
				}
                m_pEventClick = pControl;
                pControl->SetFocus();
                SetCapture();
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_BUTTONDOWN;
                event.pSender = pControl;
                event.wParam = wParam;
                event.lParam = lParam;
                event.ptMouse = pt;
                event.wKeyState = (WORD)wParam;
                event.dwTimestamp = fn_GetTickCount();
                pControl->Event(event);
#ifdef ZGUI_USE_ANIMATION
				// No need to burden user with 3D animations
				_anim.cancelJobs();
#endif
            }
            break;
        case WM_LBUTTONDBLCLK:
            {
                fn_SetFocus(_hwndPaint);
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                m_ptLastMousePos = pt;
                Control* pControl = FindControl(pt);
				if (pControl == 0 || pControl->getManager() != this) {
					break;
				}
                SetCapture();
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_DBLCLICK;
                event.pSender = pControl;
                event.ptMouse = pt;
                event.wKeyState = (WORD)wParam;
                event.dwTimestamp = fn_GetTickCount();
                pControl->Event(event);
                m_pEventClick = pControl;
            }
            break;
        case WM_LBUTTONUP:
            {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                m_ptLastMousePos = pt;
                if( m_pEventClick == 0 ) break;
                ReleaseCapture();
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_BUTTONUP;
                event.pSender = m_pEventClick;
                event.wParam = wParam;
                event.lParam = lParam;
                event.ptMouse = pt;
                event.wKeyState = (WORD)wParam;
                event.dwTimestamp = fn_GetTickCount();
                m_pEventClick->Event(event);
                m_pEventClick = 0;
            }
            break;
        case WM_RBUTTONDOWN:
            {
                fn_SetFocus(_hwndPaint);
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                m_ptLastMousePos = pt;
                Control* pControl = FindControl(pt);
				if (pControl == 0 || pControl->getManager() != this) {
					break;
				}
                pControl->SetFocus();
                SetCapture();
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_RBUTTONDOWN;
                event.pSender = pControl;
                event.wParam = wParam;
                event.lParam = lParam;
                event.ptMouse = pt;
                event.wKeyState = (WORD)wParam;
                event.dwTimestamp = fn_GetTickCount();
                pControl->Event(event);
                m_pEventClick = pControl;
            }
            break;
        case WM_CONTEXTMENU:
            {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                fn_ScreenToClient(_hwndPaint, &pt);
                m_ptLastMousePos = pt;
                if( m_pEventClick == 0 ) break;
                ReleaseCapture();
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_CONTEXTMENU;
                event.pSender = m_pEventClick;
                event.ptMouse = pt;
                event.wKeyState = (WORD)wParam;
                event.lParam = (LPARAM)m_pEventClick;
                event.dwTimestamp = fn_GetTickCount();
                m_pEventClick->Event(event);
                m_pEventClick = 0;
            }
            break;
        case WM_MOUSEWHEEL:
            {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                fn_ScreenToClient(_hwndPaint, &pt);
                m_ptLastMousePos = pt;
                Control* pControl = FindControl(pt);
				if (pControl == 0 || pControl->getManager() != this) {
					break;
				}
                int zDelta = (int) (short) HIWORD(wParam);
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_SCROLLWHEEL;
                event.pSender = pControl;
                event.wParam = MAKELPARAM(zDelta < 0 ? SB_LINEDOWN : SB_LINEUP, 0);
                event.lParam = lParam;
                event.wKeyState = MapKeyState();
                event.dwTimestamp = fn_GetTickCount();
                pControl->Event(event);

                // Let's make sure that the scroll item below the cursor is the same as before...
                fn_SendMessageW(_hwndPaint, WM_MOUSEMOVE, 0, (LPARAM) MAKELPARAM(m_ptLastMousePos.x, m_ptLastMousePos.y));
            }
            break;
        case WM_CHAR:
            {
                if( m_pFocus == 0 ) break;
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_CHAR;
                event.chKey = (TCHAR)wParam;
                event.ptMouse = m_ptLastMousePos;
                event.wKeyState = MapKeyState();
                event.dwTimestamp = fn_GetTickCount();
                m_pFocus->Event(event);
            }
            break;
        case WM_KEYDOWN:
            {
                if( m_pFocus == 0 ) break;
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_KEYDOWN;
                event.chKey = (TCHAR)wParam;
                event.ptMouse = m_ptLastMousePos;
                event.wKeyState = MapKeyState();
                event.dwTimestamp = fn_GetTickCount();
                m_pFocus->Event(event);
                m_pEventKey = m_pFocus;
            }
            break;
        case WM_KEYUP:
            {
                if( m_pEventKey == 0 ) break;
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_KEYUP;
                event.chKey = (TCHAR)wParam;
                event.ptMouse = m_ptLastMousePos;
                event.wKeyState = MapKeyState();
                event.dwTimestamp = fn_GetTickCount();
                m_pEventKey->Event(event);
                m_pEventKey = 0;
            }
            break;
        case WM_SETCURSOR:
            {
				if (LOWORD(lParam) != HTCLIENT) {
					break;
				}
				if (m_bMouseCapture) {
					return true;
				}

                POINT pt;
                __stosb((uint8_t*)&pt, 0, sizeof(pt));
                fn_GetCursorPos(&pt);
                fn_ScreenToClient(_hwndPaint, &pt);
                Control* pControl = FindControl(pt);
                if( pControl == 0 ) break;
                if( (pControl->GetControlFlags() & UIFLAG_SETCURSOR) == 0 ) break;
                TEventUI event;
                __stosb((uint8_t*)&event, 0, sizeof(event));
                event.Type = UIEVENT_SETCURSOR;
                event.wParam = wParam;
                event.lParam = lParam;
                event.ptMouse = pt;
                event.wKeyState = MapKeyState();
                event.dwTimestamp = fn_GetTickCount();
                pControl->Event(event);
            }
            return true;
        case WM_NOTIFY:
            {
                LPNMHDR lpNMHDR = (LPNMHDR) lParam;
                if (lpNMHDR != 0) {
                    lRes = fn_SendMessageW(lpNMHDR->hwndFrom, OCM__BASE + uMsg, wParam, lParam);
                }
                return true;
            }
            break;
        case WM_COMMAND:
            {
                if( lParam == 0 ) break;
                HWND hWndChild = (HWND) lParam;
                lRes = fn_SendMessageW(hWndChild, OCM__BASE + uMsg, wParam, lParam);
                return true;
            }
            break;
        case WM_CTLCOLOREDIT:
	    case WM_CTLCOLORSTATIC:
            {
			    // Refer To: http://msdn.microsoft.com/en-us/library/bb761691(v=vs.85).aspx
			    // Read-only or disabled edit controls do not send the WM_CTLCOLOREDIT message; instead, they send the WM_CTLCOLORSTATIC message.
                if( lParam == 0 ) break;
                HWND hWndChild = (HWND) lParam;
                lRes = fn_SendMessageW(hWndChild, OCM__BASE + uMsg, wParam, lParam);
                return true;
            }
            break;
        default:
            break;
    }

    pMsg = 0;
    while (pMsg = _asyncNotify[0]) {
        _asyncNotify.remove(0);
        if (pMsg->pSender != 0) {
			if (pMsg->pSender->OnNotify) {
				pMsg->pSender->OnNotify(pMsg);
			}
        }
        for (int j = 0; j < _notifiers.size(); j++ ) {
            _notifiers.getUnchecked(j)->Notify(*pMsg);
        }
        delete pMsg;
    }

    return false;
}

void PaintManager::NeedUpdate()
{
    m_bUpdateNeeded = true;
}

void PaintManager::Invalidate(RECT& rcItem)
{
    fn_InvalidateRect(_hwndPaint, &rcItem, FALSE);
}

bool PaintManager::attachRootControl(Control* pControl)
{
    zgui_assert(fn_IsWindow(_hwndPaint));
    // Reset any previous attachment
    SetFocus(0);
    m_pEventKey = 0;
    m_pEventHover = 0;
    m_pEventClick = 0;
    // Remove the existing control-tree. We might have gotten inside this function as
    // a result of an event fired or similar, so we cannot just delete the objects and
    // pull the internal memory of the calling code. We'll delay the cleanup.
    if (_pRootControl != 0) {
        _postPaintControls.clear();
        AddDelayedCleanup(_pRootControl);
    }
    // Set the window root element
    _pRootControl = pControl;
    // Go ahead...
    m_bUpdateNeeded = true;
    m_bFirstLayout = true;
    m_bFocusNeeded = true;
    // Initiate all control
    return initControls(pControl);
}

bool PaintManager::initControls(Control* pControl, Control* pParent /*= 0*/)
{
    zgui_assert(pControl);
	if (pControl == 0) {
		return false;
	}
    pControl->SetManager(this, pParent != 0 ? pParent : pControl->GetParent(), true);
    pControl->FindControl(__FindControlFromNameHash, this, UIFIND_ALL);
    return true;
}

void PaintManager::ReapObjects(Control* pControl)
{
	if (pControl == m_pEventKey) {
		m_pEventKey = 0;
	}
	if (pControl == m_pEventHover) {
		m_pEventHover = 0;
	}
	if (pControl == m_pEventClick) {
		m_pEventClick = 0;
	}
	if (pControl == m_pFocus) {
		m_pFocus = 0;
	}
    KillTimer(pControl);
    const String& sName = pControl->GetName();
    if (!sName.isEmpty()) {
        if (pControl == FindControl(sName)) {
            m_mNameHash.Remove(sName);
        }
    }
    
    for (int i = 0; i < _asyncNotify.size(); ++i) {
        TNotifyUI* pMsg = _asyncNotify.getUnchecked(i);
        if (pMsg->pSender == pControl) {
            pMsg->pSender = 0;
        }
    }    
}

bool PaintManager::AddOptionGroup(const String& pStrGroupName, Control* pControl)
{
    LPVOID lp = m_mOptionGroup.Find(pStrGroupName);
    if (lp != 0) {
		Array<void*>* aOptionGroup = static_cast<Array<void*>*>(lp);
        for (int i = 0; i < aOptionGroup->size(); ++i) {
            if (static_cast<Control*>(aOptionGroup->getUnchecked(i)) == pControl) {
                return false;
            }
        }
        aOptionGroup->add(pControl);
    }
    else {
		Array<void*>* aOptionGroup = new Array<void*>();
        aOptionGroup->add(pControl);
        m_mOptionGroup.Insert(pStrGroupName, aOptionGroup);
    }
    return true;
}

Array<void*>* PaintManager::GetOptionGroup(const String& pStrGroupName)
{
    LPVOID lp = m_mOptionGroup.Find(pStrGroupName);
    if (lp != 0) {
		return static_cast<Array<void*>*>(lp);
    }
    return 0;
}

void PaintManager::RemoveOptionGroup(const String& pStrGroupName, Control* pControl)
{
	LPVOID lp = m_mOptionGroup.Find(pStrGroupName);
	if (lp) {
		Array<void*>* aOptionGroup = static_cast<Array<void*>*>(lp);
        if (aOptionGroup == 0) {
            return;
        }
		for (int i = 0; i < aOptionGroup->size(); ++i) {
			if (static_cast<Control*>(aOptionGroup->getUnchecked(i)) == pControl) {
				aOptionGroup->remove(i);
				break;
			}
		}
		if (aOptionGroup->size() == 0) {
			delete aOptionGroup;
			m_mOptionGroup.Remove(pStrGroupName);
		}
	}
}

void PaintManager::RemoveAllOptionGroups()
{
	Array<void*>* aOptionGroup;
	for (int i = 0; i< m_mOptionGroup.GetSize(); ++i) {
		const String& key = m_mOptionGroup.GetAt(i);
		if (!key.isEmpty()) {
			aOptionGroup = static_cast<Array<void*>*>(m_mOptionGroup.Find(key));
			delete aOptionGroup;
		}
	}
	m_mOptionGroup.RemoveAll();
}

void PaintManager::messageLoop()
{
    MSG msg;
    __stosb((uint8_t*)&msg, 0, sizeof(msg));
    while (fn_GetMessageW(&msg, 0, 0, 0)) {
        if (!PaintManager::TranslateMessage(&msg)) {
            fn_TranslateMessage(&msg);
            fn_DispatchMessageW(&msg);
        }
    }
}

void PaintManager::Term()
{
}

Control* PaintManager::GetFocus() const
{
    return m_pFocus;
}

void PaintManager::SetFocus(Control* pControl)
{
    // Paint manager window has focus?
    HWND hFocusWnd = fn_GetFocus();
    if (hFocusWnd != _hwndPaint && pControl != m_pFocus) {
        fn_SetFocus(_hwndPaint);
    }
    // Already has focus?
    if (pControl == m_pFocus) {
        return;
    }
    // Remove focus from old control
    if (m_pFocus != 0) {
        TEventUI event;
        __stosb((uint8_t*)&event, 0, sizeof(event));
        event.Type = UIEVENT_KILLFOCUS;
        event.pSender = pControl;
        event.dwTimestamp = fn_GetTickCount();
        m_pFocus->Event(event);
        SendNotify(m_pFocus, "killfocus");
        m_pFocus = 0;
    }

    // Set focus to new control
    if (pControl != 0 && pControl->getManager() == this && pControl->IsVisible() && pControl->IsEnabled()) {
        m_pFocus = pControl;
        TEventUI event;
        __stosb((uint8_t*)&event, 0, sizeof(event));
        event.Type = UIEVENT_SETFOCUS;
        event.pSender = pControl;
        event.dwTimestamp = fn_GetTickCount();
        m_pFocus->Event(event);
        SendNotify(m_pFocus, "setfocus");
    }
}

void PaintManager::SetFocusNeeded(Control* pControl)
{
    fn_SetFocus(_hwndPaint);
    if( pControl == 0 ) return;
    if( m_pFocus != 0 ) {
        TEventUI event;
        __stosb((uint8_t*)&event, 0, sizeof(event));
        event.Type = UIEVENT_KILLFOCUS;
        event.pSender = pControl;
        event.dwTimestamp = fn_GetTickCount();
        m_pFocus->Event(event);
        SendNotify(m_pFocus, "killfocus");
        m_pFocus = 0;
    }
    FINDTABINFO info;
    __stosb((uint8_t*)&info, 0, sizeof(info));
    info.pFocus = pControl;
    info.bForward = false;
    m_pFocus = _pRootControl->FindControl(__FindControlFromTab, &info, UIFIND_VISIBLE | UIFIND_ENABLED | UIFIND_ME_FIRST);
    m_bFocusNeeded = true;
    if( _pRootControl != 0 ) _pRootControl->NeedUpdate();
}

bool PaintManager::SetTimer(Control* pControl, UINT nTimerID, UINT uElapse)
{
    zgui_assert(pControl!=0);
    zgui_assert(uElapse>0);
    for (int i = 0; i< _timers.size(); ++i) {
        TIMERINFO* pTimer = _timers.getUnchecked(i);
        if (pTimer->pSender == pControl && pTimer->hWnd == _hwndPaint && pTimer->nLocalID == nTimerID) {
            if (pTimer->bKilled == true) {
                if (fn_SetTimer(_hwndPaint, pTimer->uWinTimer, uElapse, 0)) {
                    pTimer->bKilled = false;
                    return true;
                }
                return false;
            }
            return false;
        }
    }

    m_uTimerID = (++m_uTimerID) % 0xFF;
    if (!fn_SetTimer(_hwndPaint, m_uTimerID, uElapse, 0)) {
        return false;
    }
    TIMERINFO* pTimer = new TIMERINFO;
    if (pTimer == 0) {
        return false;
    }
    pTimer->hWnd = _hwndPaint;
    pTimer->pSender = pControl;
    pTimer->nLocalID = nTimerID;
    pTimer->uWinTimer = m_uTimerID;
    pTimer->bKilled = false;
	_timers.add(pTimer);
	return true;
}

bool PaintManager::KillTimer(Control* pControl, UINT nTimerID)
{
    zgui_assert(pControl != 0);
    for (int i = 0; i< _timers.size(); ++i) {
        TIMERINFO* pTimer = _timers.getUnchecked(i);
        if (pTimer->pSender == pControl && pTimer->hWnd == _hwndPaint && pTimer->nLocalID == nTimerID) {
            if (pTimer->bKilled == false) {
                if (fn_IsWindow(_hwndPaint)) {
                    fn_KillTimer(pTimer->hWnd, pTimer->uWinTimer);
                }
                pTimer->bKilled = true;
                return true;
            }
        }
    }
    return false;
}

void PaintManager::KillTimer(Control* pControl)
{
    zgui_assert(pControl!=0);
    int count = _timers.size();
    for (int i = 0, j = 0; i < count; ++i) {
        TIMERINFO* pTimer = _timers.getUnchecked(i - j);
        if (pTimer->pSender == pControl && pTimer->hWnd == _hwndPaint) {
            if (pTimer->bKilled == false) {
                fn_KillTimer(pTimer->hWnd, pTimer->uWinTimer);
            }
            delete pTimer;
            _timers.remove(i - j);
            ++j;
        }
    }
}

void PaintManager::RemoveAllTimers()
{
    for (int i = 0; i < _timers.size(); ++i) {
        TIMERINFO* pTimer = _timers.getUnchecked(i);
        if (pTimer->hWnd == _hwndPaint) {
            if (pTimer->bKilled == false) {
                if (fn_IsWindow(_hwndPaint)) {
                    fn_KillTimer(_hwndPaint, pTimer->uWinTimer);
                }
            }
            delete pTimer;
        }
    }
    _timers.clear();
}

void PaintManager::SetCapture()
{
    fn_SetCapture(_hwndPaint);
    m_bMouseCapture = true;
}

void PaintManager::ReleaseCapture()
{
    fn_ReleaseCapture();
    m_bMouseCapture = false;
}

bool PaintManager::IsCaptured()
{
    return m_bMouseCapture;
}

bool PaintManager::SetNextTabControl(bool bForward)
{
    // If we're in the process of restructuring the layout we can delay the
    // focus calulation until the next repaint.
    if( m_bUpdateNeeded && bForward ) {
        m_bFocusNeeded = true;
        fn_InvalidateRect(_hwndPaint, 0, FALSE);
        return true;
    }
    // Find next/previous tabbable control
    FINDTABINFO info1;
    __stosb((uint8_t*)&info1, 0, sizeof(info1));
    info1.pFocus = m_pFocus;
    info1.bForward = bForward;
    Control* pControl = _pRootControl->FindControl(__FindControlFromTab, &info1, UIFIND_VISIBLE | UIFIND_ENABLED | UIFIND_ME_FIRST);
    if( pControl == 0 ) {  
        if( bForward ) {
            // Wrap around
            FINDTABINFO info2;
            __stosb((uint8_t*)&info2, 0, sizeof(info2));
            info2.pFocus = bForward ? 0 : info1.pLast;
            info2.bForward = bForward;
            pControl = _pRootControl->FindControl(__FindControlFromTab, &info2, UIFIND_VISIBLE | UIFIND_ENABLED | UIFIND_ME_FIRST);
        }
        else {
            pControl = info1.pLast;
        }
    }
    if( pControl != 0 ) {
        SetFocus(pControl);
    }
    m_bFocusNeeded = false;
    return true;
}

void PaintManager::AddNotifier(INotifyUI* pNotifier)
{
    zgui_assert(_notifiers.indexOf(pNotifier) < 0);
    _notifiers.add(pNotifier);
}

bool PaintManager::RemoveNotifier(INotifyUI* pNotifier)
{
    for( int i = 0; i < _notifiers.size(); i++ ) {
        if (_notifiers.getUnchecked(i) == pNotifier) {
            return (_notifiers.remove(i) != 0);
        }
    }
    return false;
}

void PaintManager::AddPreMessageFilter(IMessageFilterUI* pFilter)
{
    zgui_assert(_preMessageFilters.indexOf(pFilter) < 0);
    _preMessageFilters.add(pFilter);
}

bool PaintManager::RemovePreMessageFilter(IMessageFilterUI* pFilter)
{
    for (int i = 0; i < _preMessageFilters.size(); ++i) {
        if (_preMessageFilters.getUnchecked(i) == pFilter) {
            return (_preMessageFilters.remove(i) != 0);
        }
    }
    return false;
}

void PaintManager::AddMessageFilter(IMessageFilterUI* pFilter)
{
    zgui_assert(_messageFilters.indexOf(pFilter) < 0);
    _messageFilters.add(pFilter);
}

bool PaintManager::RemoveMessageFilter(IMessageFilterUI* pFilter)
{
    for (int i = 0; i < _messageFilters.size(); ++i) {
        if (_messageFilters.getUnchecked(i) == pFilter) {
            return (_messageFilters.remove(i) != 0);
        }
    }
    return false;
}

int PaintManager::GetPostPaintCount() const
{
    return _postPaintControls.size();
}

void PaintManager::AddPostPaint(Control* pControl)
{
    zgui_assert(_postPaintControls.indexOf(pControl) < 0);
    _postPaintControls.add(pControl);
}

bool PaintManager::RemovePostPaint(Control* pControl)
{
    for (int i = 0; i < _postPaintControls.size(); ++i) {
        if (_postPaintControls.getUnchecked(i) == pControl) {
            return (_postPaintControls.remove(i) != 0);
        }
    }
    return false;
}

void PaintManager::SetPostPaintIndex(Control* pControl, int iIndex)
{
    RemovePostPaint(pControl);
    _postPaintControls.insert(iIndex, pControl);
}

void PaintManager::AddDelayedCleanup(Control* pControl)
{
    pControl->SetManager(this, 0, false);
    _delayedCleanup.add(pControl);
    fn_PostMessageW(_hwndPaint, WM_APP + 1, 0, 0L);
}

void PaintManager::SendNotify(Control* pControl, const String& pstrMessage, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/, bool bAsync /*= false*/)
{
    TNotifyUI Msg;
    Msg.pSender = pControl;
    Msg.sType = pstrMessage;
    Msg.wParam = wParam;
    Msg.lParam = lParam;
    SendNotify(Msg, bAsync);
}

void PaintManager::SendNotify(TNotifyUI& Msg, bool bAsync /*= false*/)
{
    Msg.ptMouse = m_ptLastMousePos;
    Msg.dwTimestamp = fn_GetTickCount();
    
    if (m_bUsedVirtualWnd) {
        Msg.sVirtualWnd = Msg.pSender->GetVirtualWnd();
    }

    if (!bAsync) {
        // Send to all listeners
        if (Msg.pSender != 0) {
            if (Msg.pSender->OnNotify) {
                Msg.pSender->OnNotify(&Msg);
            }
        }
        for (int i = 0; i < _notifiers.size(); ++i) {
            _notifiers.getUnchecked(i)->Notify(Msg);
        }
    }
    else {
        TNotifyUI *pMsg = new TNotifyUI;
        pMsg->pSender = Msg.pSender;
        pMsg->sType = Msg.sType;
        pMsg->wParam = Msg.wParam;
        pMsg->lParam = Msg.lParam;
        pMsg->ptMouse = Msg.ptMouse;
        pMsg->dwTimestamp = Msg.dwTimestamp;
        _asyncNotify.add(pMsg);
    }
}

bool PaintManager::UseParentResource(PaintManager* pm)
{
    if (pm == 0) {
        m_pParentResourcePM = 0;
        return true;
    }
	if (pm == this) {
		return false;
	}

    PaintManager* pParentPM = pm->GetParentResource();
    while (pParentPM != 0) {
		if (pParentPM == this) {
			return false;
		}
        pParentPM = pParentPM->GetParentResource();
    }
    m_pParentResourcePM = pm;
    return true;
}

PaintManager* PaintManager::GetParentResource() const
{
    return m_pParentResourcePM;
}

DWORD PaintManager::GetDefaultDisabledColor() const
{
	if (m_pParentResourcePM) {
		return m_pParentResourcePM->GetDefaultDisabledColor();
	}
    return m_dwDefaultDisabledColor;
}

void PaintManager::SetDefaultDisabledColor(DWORD dwColor)
{
    m_dwDefaultDisabledColor = dwColor;
}

DWORD PaintManager::GetDefaultFontColor() const
{
	if (m_pParentResourcePM) {
		return m_pParentResourcePM->GetDefaultFontColor();
	}
    return m_dwDefaultFontColor;
}

void PaintManager::SetDefaultFontColor(DWORD dwColor)
{
    m_dwDefaultFontColor = dwColor;
}

DWORD PaintManager::GetDefaultLinkFontColor() const
{
    if (m_pParentResourcePM != 0) {
        return m_pParentResourcePM->GetDefaultLinkFontColor();
    }

    return _dwDefaultLinkFontColor;
}

void PaintManager::SetDefaultLinkFontColor(DWORD dwColor)
{
    _dwDefaultLinkFontColor = dwColor;
}

DWORD PaintManager::GetDefaultLinkHoverFontColor() const
{
    if (m_pParentResourcePM != 0) {
        return m_pParentResourcePM->GetDefaultLinkHoverFontColor();
    }

    return _dwDefaultLinkHoverFontColor;
}

void PaintManager::SetDefaultLinkHoverFontColor(DWORD dwColor)
{
    _dwDefaultLinkHoverFontColor = dwColor;
}

DWORD PaintManager::GetDefaultSelectedBkColor() const
{
    if (m_pParentResourcePM != 0) {
        return m_pParentResourcePM->GetDefaultSelectedBkColor();
    }

    return m_dwDefaultSelectedBkColor;
}

void PaintManager::SetDefaultSelectedBkColor(DWORD dwColor)
{
    m_dwDefaultSelectedBkColor = dwColor;
}

TFontInfo* PaintManager::GetDefaultFontInfo()
{
    if (m_pParentResourcePM != 0) {
        return m_pParentResourcePM->GetDefaultFontInfo();
    }

    if (m_DefaultFontInfo.tm.tmHeight == 0) {
        HFONT hOldFont = (HFONT) fn_SelectObject(_hdcPaint, m_DefaultFontInfo.hFont);
        fn_GetTextMetricsW(_hdcPaint, &m_DefaultFontInfo.tm);
        fn_SelectObject(_hdcPaint, hOldFont);
    }
    return &m_DefaultFontInfo;
}

void PaintManager::SetDefaultFont(const String& fontName, int nSize, bool bBold, bool bUnderline, bool bItalic)
{
    LOGFONTW lf;
    __stosb((uint8_t*)&lf, 0, sizeof(lf));
    fn_GetObjectW(fn_GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    fontName.copyToUTF16(lf.lfFaceName, sizeof(lf.lfFaceName) - 2);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfHeight = -nSize;
    if (bBold) {
        lf.lfWeight += FW_BOLD;
    }
    if (bUnderline) {
        lf.lfUnderline = TRUE;
    }
    if (bItalic) {
        lf.lfItalic = TRUE;
    }
    HFONT hFont = fn_CreateFontIndirectW(&lf);
    if (hFont == 0) {
        return;
    }

    fn_DeleteObject(m_DefaultFontInfo.hFont);
    m_DefaultFontInfo.hFont = hFont;
    m_DefaultFontInfo.sFontName = fontName;
    m_DefaultFontInfo.iSize = nSize;
    m_DefaultFontInfo.bBold = bBold;
    m_DefaultFontInfo.bUnderline = bUnderline;
    m_DefaultFontInfo.bItalic = bItalic;
    __stosb((uint8_t*)&m_DefaultFontInfo.tm, 0, sizeof(m_DefaultFontInfo.tm));
    if (_hdcPaint) {
        HFONT hOldFont = (HFONT)fn_SelectObject(_hdcPaint, hFont);
        fn_GetTextMetricsW(_hdcPaint, &m_DefaultFontInfo.tm);
        fn_SelectObject(_hdcPaint, hOldFont);
    }
}

DWORD PaintManager::GetCustomFontCount() const
{
    return _customFonts.size();
}

HFONT PaintManager::AddFont(const String& pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic)
{
    LOGFONTW lf;
    __stosb((uint8_t*)&lf, 0, sizeof(lf));
    fn_GetObjectW(fn_GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    pStrFontName.copyToUTF16(lf.lfFaceName, sizeof(lf.lfFaceName) - 2);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfHeight = -nSize;
    if (bBold) {
        lf.lfWeight += FW_BOLD;
    }
    if (bUnderline) {
        lf.lfUnderline = TRUE;
    }
    if (bItalic) {
        lf.lfItalic = TRUE;
    }
    HFONT hFont = fn_CreateFontIndirectW(&lf);
    if (hFont == 0) {
        return 0;
    }

    TFontInfo* pFontInfo = new TFontInfo;
    if (!pFontInfo) {
        return false;
    }

    pFontInfo->hFont = hFont;
    pFontInfo->sFontName = pStrFontName;
    pFontInfo->iSize = nSize;
    pFontInfo->bBold = bBold;
    pFontInfo->bUnderline = bUnderline;
    pFontInfo->bItalic = bItalic;
    if (_hdcPaint) {
        HFONT hOldFont = (HFONT) fn_SelectObject(_hdcPaint, hFont);
        fn_GetTextMetricsW(_hdcPaint, &pFontInfo->tm);
        fn_SelectObject(_hdcPaint, hOldFont);
    }
	_customFonts.add(pFontInfo);

    return hFont;
}

HFONT PaintManager::AddFontAt(int index, LPCTSTR pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic)
{
    LOGFONTW lf;
    __stosb((uint8_t*)&lf, 0, sizeof(lf));
    fn_GetObjectW(fn_GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    fn_lstrcpyW(lf.lfFaceName, pStrFontName);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfHeight = -nSize;
    if (bBold) {
        lf.lfWeight += FW_BOLD;
    }
    if (bUnderline) {
        lf.lfUnderline = TRUE;
    }
    if (bItalic) {
        lf.lfItalic = TRUE;
    }
    HFONT hFont = fn_CreateFontIndirectW(&lf);
    if (hFont == 0) {
        return 0;
    }

    TFontInfo* pFontInfo = new TFontInfo;
    if (!pFontInfo) {
        return false;
    }

    __stosb((uint8_t*)pFontInfo, 0, sizeof(TFontInfo));
    pFontInfo->hFont = hFont;
    pFontInfo->sFontName = pStrFontName;
    pFontInfo->iSize = nSize;
    pFontInfo->bBold = bBold;
    pFontInfo->bUnderline = bUnderline;
    pFontInfo->bItalic = bItalic;
    
    if (_hdcPaint) {
        HFONT hOldFont = (HFONT) fn_SelectObject(_hdcPaint, hFont);
        fn_GetTextMetricsW(_hdcPaint, &pFontInfo->tm);
        fn_SelectObject(_hdcPaint, hOldFont);
    }
	_customFonts.insert(index, pFontInfo);

    return hFont;
}

HFONT PaintManager::GetFont(int index)
{
    if (index < 0 || index >= _customFonts.size()) {
        return GetDefaultFontInfo()->hFont;
    }
    TFontInfo* pFontInfo = _customFonts.getUnchecked(index);
    return pFontInfo->hFont;
}

HFONT PaintManager::GetFont(const String& pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic)
{
    TFontInfo* pFontInfo = 0;
    for (int it = 0; it < _customFonts.size(); ++it) {
        pFontInfo = _customFonts.getUnchecked(it);
        if (pFontInfo->sFontName == pStrFontName && pFontInfo->iSize == nSize && pFontInfo->bBold == bBold && pFontInfo->bUnderline == bUnderline && pFontInfo->bItalic == bItalic) {
            return pFontInfo->hFont;
        }
    }
    if (m_pParentResourcePM) {
        return m_pParentResourcePM->GetFont(pStrFontName, nSize, bBold, bUnderline, bItalic);
    }
    return 0;
}

bool PaintManager::FindFont(HFONT hFont)
{
    TFontInfo* pFontInfo = 0;
    for (int it = 0; it < _customFonts.size(); ++it) {
        pFontInfo = _customFonts.getUnchecked(it);
        if (pFontInfo->hFont == hFont) {
            return true;
        }
    }
    if (m_pParentResourcePM) {
        return m_pParentResourcePM->FindFont(hFont);
    }

    return false;
}

bool PaintManager::FindFont(const String& pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic)
{
    TFontInfo* pFontInfo = 0;
    for (int it = 0; it < _customFonts.size(); ++it) {
        pFontInfo = _customFonts.getUnchecked(it);
        if (pFontInfo->sFontName == pStrFontName && pFontInfo->iSize == nSize && pFontInfo->bBold == bBold && pFontInfo->bUnderline == bUnderline && pFontInfo->bItalic == bItalic) {
            return true;
        }
    }
    if (m_pParentResourcePM) {
        return m_pParentResourcePM->FindFont(pStrFontName, nSize, bBold, bUnderline, bItalic);
    }

    return false;
}

int PaintManager::GetFontIndex(HFONT hFont)
{
    TFontInfo* pFontInfo = 0;
    for (int it = 0; it < _customFonts.size(); ++it) {
        pFontInfo = _customFonts.getUnchecked(it);
        if (pFontInfo->hFont == hFont) {
            return it;
        }
    }
    return -1;
}

int PaintManager::GetFontIndex(const String& pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic)
{
    TFontInfo* pFontInfo = 0;
    for (int it = 0; it < _customFonts.size(); ++it) {
        pFontInfo = _customFonts.getUnchecked(it);
        if (pFontInfo->sFontName == pStrFontName && pFontInfo->iSize == nSize && pFontInfo->bBold == bBold && pFontInfo->bUnderline == bUnderline && pFontInfo->bItalic == bItalic) {
            return it;
        }
    }
    return -1;
}

bool PaintManager::RemoveFont(HFONT hFont)
{
    TFontInfo* pFontInfo = 0;
    for (int it = 0; it < _customFonts.size(); ++it) {
        pFontInfo = _customFonts.getUnchecked(it);
        if (pFontInfo->hFont == hFont) {
            fn_DeleteObject(pFontInfo->hFont);
            delete pFontInfo;
            return (_customFonts.remove(it) != 0);
        }
    }

    return false;
}

bool PaintManager::RemoveFontAt(int index)
{
	if (index < 0 || index >= _customFonts.size()) {
		return false;
	}
    TFontInfo* pFontInfo = _customFonts.getUnchecked(index);
    fn_DeleteObject(pFontInfo->hFont);
    delete pFontInfo;
	return (_customFonts.remove(index) != 0);
}

void PaintManager::RemoveAllFonts()
{
    TFontInfo* pFontInfo;
    for (int it = 0; it < _customFonts.size(); ++it) {
        pFontInfo = _customFonts.getUnchecked(it);
        fn_DeleteObject(pFontInfo->hFont);
        delete pFontInfo;
    }
    _customFonts.clear();
}

TFontInfo* PaintManager::GetFontInfo(int index)
{
	if (index < 0 || index >= _customFonts.size()) {
		if (m_pParentResourcePM != 0) {
			return m_pParentResourcePM->GetFontInfo(index);
		}
		return GetDefaultFontInfo();
	}
    TFontInfo* pFontInfo = _customFonts.getUnchecked(index);
    if (pFontInfo->tm.tmHeight == 0) {
        HFONT hOldFont = (HFONT) fn_SelectObject(_hdcPaint, pFontInfo->hFont);
        fn_GetTextMetricsW(_hdcPaint, &pFontInfo->tm);
        fn_SelectObject(_hdcPaint, hOldFont);
    }
    return pFontInfo;
}

TFontInfo* PaintManager::GetFontInfo(HFONT hFont)
{
    TFontInfo* pFontInfo = 0;
    for (int it = 0; it < _customFonts.size(); ++it) {
        pFontInfo = _customFonts.getUnchecked(it);
        if (pFontInfo->hFont == hFont) {
            if (pFontInfo->tm.tmHeight == 0) {
                HFONT hOldFont = (HFONT) fn_SelectObject(_hdcPaint, pFontInfo->hFont);
                fn_GetTextMetricsW(_hdcPaint, &pFontInfo->tm);
                fn_SelectObject(_hdcPaint, hOldFont);
            }
            return pFontInfo;
        }
    }

	if (m_pParentResourcePM) {
		return m_pParentResourcePM->GetFontInfo(hFont);
	}
    return GetDefaultFontInfo();
}

const TImageInfo* PaintManager::getImage(const String& bitmap)
{
    TImageInfo* data = static_cast<TImageInfo*>(m_mImageHash.Find(bitmap));
    if (data == 0 && m_pParentResourcePM != 0) {
        return m_pParentResourcePM->getImage(bitmap);
    }
    else {
        return data;
    }
}

const TImageInfo* PaintManager::getImageEx(const String& bitmap)
{
    TImageInfo* data = static_cast<TImageInfo*>(m_mImageHash.Find(bitmap));
    if (data == 0) {
		return addImage(bitmap);
    }
    return data;
}

const TImageInfo* PaintManager::addImage(const String& bitmap)
{
    TImageInfo* data = 0;
    
    data = RenderEngine::loadImage(bitmap);

    if (data == 0) {
        return 0;
    }

    if (!m_mImageHash.Insert(bitmap, data)) {
        fn_DeleteObject(data->hBitmap);
        delete data;
		data = 0;
    }

    return data;
}

const TImageInfo* PaintManager::addImage(const String& bitmapName, HBITMAP hBitmap, int iWidth, int iHeight)
{
    if (hBitmap == 0 || iWidth <= 0 || iHeight <= 0) {
        return 0;
    }

    TImageInfo* data = new TImageInfo;
    data->hBitmap = hBitmap;
    data->width = iWidth;
    data->height = iHeight;
    if (!m_mImageHash.Insert(bitmapName, data)) {
        fn_DeleteObject(hBitmap);
        delete data;
    }

    return data;
}

bool PaintManager::removeImage(const String& bitmap)
{
    const TImageInfo* data = getImage(bitmap);
	if (data == 0) {
		return false;
	}

    RenderEngine::FreeImage(data) ;

    return m_mImageHash.Remove(bitmap);
}

void PaintManager::removeAllImages()
{
    TImageInfo* data;
    for (int i = 0; i< m_mImageHash.GetSize(); ++i) {
		const String& key = m_mImageHash.GetAt(i);
		if (key.isEmpty()) {
            data = static_cast<TImageInfo*>(m_mImageHash.Find(key, false));
			if (data) {
				RenderEngine::FreeImage(data);
			}
        }
    }
	m_mImageHash.RemoveAll();
}

void PaintManager::reloadAllImages()
{
    bool bRedraw = false;
    TImageInfo* data;
    TImageInfo* pNewData;
    for (int i = 0; i < m_mImageHash.GetSize(); ++i) {
		const String& bitmap = m_mImageHash.GetAt(i);
		if (!bitmap.isEmpty()) {
            data = static_cast<TImageInfo*>(m_mImageHash.Find(bitmap));
            if (data != 0) {
                pNewData = RenderEngine::loadImage(bitmap);
                
				if (pNewData == 0) {
					continue;
				}

                if (data->hBitmap != 0) {
                    fn_DeleteObject(data->hBitmap);
                }
                data->hBitmap = pNewData->hBitmap;
				data->width = pNewData->width;
				data->height = pNewData->height;

                delete pNewData;
                bRedraw = true;
            }
        }
    }
	if (bRedraw && _pRootControl) {
		_pRootControl->Invalidate();
	}
}

void PaintManager::addDefaultAttributeList(const String& controlName, const String& controlAttrList)
{
	String* pDefaultAttr = new String(controlAttrList);

	if (pDefaultAttr != 0) {
        if (m_DefaultAttrHash.Find(controlName) == 0) {
			m_DefaultAttrHash.Set(controlName, (LPVOID)pDefaultAttr);
        }
        else {
			delete pDefaultAttr;
        }
	}
}

const String& PaintManager::getDefaultAttributeList(const String& controlName) const
{
    String* pDefaultAttr = static_cast<String*>(m_DefaultAttrHash.Find(controlName));
    if (!pDefaultAttr && m_pParentResourcePM) {
        return m_pParentResourcePM->getDefaultAttributeList(controlName);
    }
    
    if (pDefaultAttr) {
        return *pDefaultAttr;
    }
    else {
        return String::empty;
    }
}

bool PaintManager::RemoveDefaultAttributeList(LPCTSTR pStrControlName)
{
    String* pDefaultAttr = static_cast<String*>(m_DefaultAttrHash.Find(pStrControlName));
	if (!pDefaultAttr) {
		return false;
	}

    delete pDefaultAttr;
    return m_DefaultAttrHash.Remove(pStrControlName);
}

const StringPtrMap& PaintManager::GetDefaultAttribultes() const
{
	return m_DefaultAttrHash;
}

void PaintManager::RemoveAllDefaultAttributeList()
{
	String* pDefaultAttr;
	for (int i = 0; i< m_DefaultAttrHash.GetSize(); ++i) {
		const String& key = m_DefaultAttrHash.GetAt(i);
		if (!key.isEmpty()) {
			pDefaultAttr = static_cast<String*>(m_DefaultAttrHash.Find(key));
			delete pDefaultAttr;
		}
	}
	m_DefaultAttrHash.RemoveAll();
}

#ifdef ZGUI_USE_ANIMATION
bool PaintManager::addAnimationJob(Animation* pAnimation)
{
	_anim.addJob(pAnimation);
	fn_InvalidateRect(_hwndPaint, NULL, FALSE);
	return true;
}
#endif // ZGUI_USE_ANIMATION

void PaintManager::addStyle(const String& styleName, StringPairArray* pParams)
{
	_stylesMap.Insert(styleName, pParams);
}

StringPairArray* PaintManager::getStyle(const String& styleName)
{
	return static_cast<StringPairArray*>(_stylesMap.Find(styleName));
}

void PaintManager::acceptStyle(Control* pControl, const String& styleName)
{
	StringPairArray* pParams = getStyle(styleName);
	if (pParams != 0) {
		const StringArray& styleKeys = pParams->getAllKeys();
		const StringArray& styleValues = pParams->getAllValues();

		for (int n = styleKeys.size(); --n >= 0;) {
			pControl->setAttribute(styleKeys[n], styleValues[n]);
		}
	}
}

void PaintManager::removeAllStyles()
{
	for (int i = _stylesMap.GetSize(); --i >= 0;) {
		const String& key = _stylesMap.GetAt(i);
		if (!key.isEmpty()) {
			delete _stylesMap.Find(key);
		}
	}

	_stylesMap.RemoveAll();
}

Control* PaintManager::GetRoot() const
{
    zgui_assert(_pRootControl);
    return _pRootControl;
}

Control* PaintManager::FindControl(POINT pt) const
{
    zgui_assert(_pRootControl);
    return _pRootControl->FindControl(__FindControlFromPoint, &pt, UIFIND_VISIBLE | UIFIND_HITTEST | UIFIND_TOP_FIRST);
}

Control* PaintManager::FindControl(const String& pstrName) const
{
    zgui_assert(_pRootControl);
    return static_cast<Control*>(m_mNameHash.Find(pstrName));
}

Control* PaintManager::FindSubControlByPoint(Control* pParent, POINT pt) const
{
    if( pParent == 0 ) pParent = GetRoot();
    zgui_assert(pParent);
    return pParent->FindControl(__FindControlFromPoint, &pt, UIFIND_VISIBLE | UIFIND_HITTEST | UIFIND_TOP_FIRST);
}

Control* PaintManager::FindSubControlByName(Control* pParent, const String& pstrName) const
{
	if (pParent == 0) {
		pParent = GetRoot();
	}
    zgui_assert(pParent);
    return pParent->FindControl(__FindControlFromName, (LPVOID)&pstrName, UIFIND_ALL);
}

Control* PaintManager::FindSubControlByClass(Control* pParent, const String& pstrClass, int iIndex)
{
    if( pParent == 0 ) pParent = GetRoot();
    zgui_assert(pParent);
    _foundControls.resize(iIndex + 1);
    return pParent->FindControl(__FindControlFromClass, (LPVOID)&pstrClass, UIFIND_ALL);
}

Array<void*>* PaintManager::FindSubControlsByClass(Control* pParent, const String& pstrClass)
{
	if (pParent == 0) {
		pParent = GetRoot();
	}
    zgui_assert(pParent);
    _foundControls.clear();
    pParent->FindControl(__FindControlsFromClass, (LPVOID)&pstrClass, UIFIND_ALL);
    return &_foundControls;
}

Array<void*>* PaintManager::GetSubControlsByClass()
{
    return &_foundControls;
}

Control* CALLBACK PaintManager::__FindControlFromNameHash(Control* pThis, LPVOID pData)
{
    PaintManager* pManager = static_cast<PaintManager*>(pData);
    const String& sName = pThis->GetName();
    if (sName.isEmpty()) {
        return 0;
    }
    // Add this control to the hash list
    pManager->m_mNameHash.Set(sName, pThis);
    return 0; // Attempt to add all controls
}

Control* CALLBACK PaintManager::__FindControlFromCount(Control* /*pThis*/, LPVOID pData)
{
    int* pnCount = static_cast<int*>(pData);
    (*pnCount)++;
    return 0;  // Count all controls
}

Control* CALLBACK PaintManager::__FindControlFromPoint(Control* pThis, LPVOID pData)
{
    LPPOINT pPoint = static_cast<LPPOINT>(pData);
    return fn_PtInRect(&pThis->GetPos(), *pPoint) ? pThis : 0;
}

Control* CALLBACK PaintManager::__FindControlFromTab(Control* pThis, LPVOID pData)
{
    FINDTABINFO* pInfo = static_cast<FINDTABINFO*>(pData);
    if( pInfo->pFocus == pThis ) {
        if( pInfo->bForward ) pInfo->bNextIsIt = true;
        return pInfo->bForward ? 0 : pInfo->pLast;
    }
    if( (pThis->GetControlFlags() & UIFLAG_TABSTOP) == 0 ) return 0;
    pInfo->pLast = pThis;
    if( pInfo->bNextIsIt ) return pThis;
    if( pInfo->pFocus == 0 ) return pThis;
    return 0;  // Examine all controls
}

Control* CALLBACK PaintManager::__FindControlFromShortcut(Control* pThis, LPVOID pData)
{
    if (!pThis->IsVisible()) {
        return 0; 
    }
    FINDSHORTCUT* pFS = static_cast<FINDSHORTCUT*>(pData);
    if (pFS->ch == CharacterFunctions::toUpperCase(pThis->GetShortcut())) {
        pFS->bPickNext = true;
    }
    if (pThis->getClass().indexOf("LabelUI") != -1) {
        return 0;   // Labels never get focus!
    }
    return pFS->bPickNext ? pThis : 0;
}

Control* CALLBACK PaintManager::__FindControlFromUpdate(Control* pThis, LPVOID /*pData*/)
{
    return pThis->IsUpdateNeeded() ? pThis : 0;
}

Control* CALLBACK PaintManager::__FindControlFromName(Control* pThis, LPVOID pData)
{
	const String* pstrName = static_cast<const String*>(pData);
    const String& sName = pThis->GetName();
    if (sName.isEmpty()) {
        return 0;
    }
    
	return (pstrName->compareIgnoreCase(sName) == 0) ? pThis : 0;
}

Control* CALLBACK PaintManager::__FindControlFromClass(Control* pThis, LPVOID pData)
{
	const String* pstrType = static_cast<const String*>(pData);
    const String& pType = pThis->getClass();
	Array<void*>* pFoundControls = pThis->getManager()->GetSubControlsByClass();
    if (*pstrType == "*" || *pstrType == pType) {
        int iIndex = -1;
        while (pFoundControls->getUnchecked(++iIndex) != 0) ;
        if (iIndex < pFoundControls->size()) {
            pFoundControls->set(iIndex, pThis);
        }
    }
    if (pFoundControls->getLast() != 0) {
        return pThis; 
    }
    return 0;
}

Control* CALLBACK PaintManager::__FindControlsFromClass(Control* pThis, LPVOID pData)
{
	const String* pstrType = static_cast<const String*>(pData);
    const String& pType = pThis->getClass();
    if (*pstrType == "*" || *pstrType == pType)  {
        pThis->getManager()->GetSubControlsByClass()->add((LPVOID)pThis);
    }
    return 0;
}

bool PaintManager::TranslateAccelerator(LPMSG pMsg)
{
	for (int i = 0; i < _translateAccelerator.size(); ++i) {
		LRESULT lResult = _translateAccelerator.getUnchecked(i)->TranslateAccelerator(pMsg);
		if( lResult == S_OK ) return true;
	}
	return false;
}

bool PaintManager::TranslateMessage(const LPMSG pMsg)
{
	// Pretranslate Message takes care of system-wide messages, such as
	// tabbing and shortcut key-combos. We'll look for all messages for
	// each window and any child control attached.
	UINT uStyle = fn_GetWindowLongW(pMsg->hwnd, GWL_STYLE);
    UINT uChildRes = uStyle & WS_CHILD;
	LRESULT lRes = 0;

	if (uChildRes != 0) {
		HWND hWndParent = fn_GetParent(pMsg->hwnd);

		for (int i = 0; i < _preMessages.size(); ++i) {
			PaintManager* pT = _preMessages.getUnchecked(i);        
			HWND hTempParent = hWndParent;
			while (hTempParent != 0) {
				if (pMsg->hwnd == pT->GetPaintWindow() || hTempParent == pT->GetPaintWindow()) {
					if (pT->TranslateAccelerator(pMsg)) {
						return true;
					}

					if (pT->PreMessageHandler(pMsg->message, pMsg->wParam, pMsg->lParam, lRes)) {
						return true;
					}

					return false;
				}
				hTempParent = fn_GetParent(hTempParent);
			}
		}
	}
    else {
        for (int i = 0; i < _preMessages.size(); ++i) {
            PaintManager* pT = _preMessages.getUnchecked(i);
            if (pMsg->hwnd == pT->GetPaintWindow()) {
                if (pT->TranslateAccelerator(pMsg)) {
					return true;
                }

                if (pT->PreMessageHandler(pMsg->message, pMsg->wParam, pMsg->lParam, lRes)) {
					return true;
                }

				return false;
			}
		}
    }
    return false;
}

void PaintManager::AddTranslateAccelerator(ITranslateAccelerator *pTranslateAccelerator)
{
	zgui_assert(_translateAccelerator.indexOf(pTranslateAccelerator) < 0);
	_translateAccelerator.add(pTranslateAccelerator);
}

bool PaintManager::RemoveTranslateAccelerator(ITranslateAccelerator *pTranslateAccelerator)
{
	for (int i = 0; i < _translateAccelerator.size(); ++i) {
		if (static_cast<ITranslateAccelerator *>(_translateAccelerator[i]) == pTranslateAccelerator) {
			return (_translateAccelerator.remove(i) != 0);
		}
	}
	return false;
}

void PaintManager::UsedVirtualWnd(bool bUsed)
{
    m_bUsedVirtualWnd = bUsed;
}

} // namespace zgui
