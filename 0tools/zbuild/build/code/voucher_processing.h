#ifndef __ELOCKER_VOUCHERPROCESSING_H_
#define __ELOCKER_VOUCHERPROCESSING_H_

#include "alert_window.h"
#include "server_netcomm.h"

class LockerControlThread : public zgui::Thread
{
public:
    LockerControlThread();
    ~LockerControlThread();

    bool requestServer(int reqType);

    void processVoucher();
    void setPaintManager(zgui::CPaintManagerUI* pPaintMgr, HWND hWnd);

    zgui_DeclareSingleton(LockerControlThread)

private:
    void run();
    bool checkVoucherStatus();

    zgui::WaitableEvent _event;
    zgui::CPaintManagerUI* _pPaintMgr;
    HWND _hWnd;
};

#endif // __ELOCKER_VOUCHERPROCESSING_H_
