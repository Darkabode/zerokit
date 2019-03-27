zgui_ImplementSingleton(LockerControlThread)

LockerControlThread::LockerControlThread()
{

}

LockerControlThread::~LockerControlThread()
{

}

void LockerControlThread::setPaintManager(zgui::CPaintManagerUI* pPaintMgr, HWND hWnd)
{
    _pPaintMgr = pPaintMgr;
    _hWnd = hWnd;
}

void LockerControlThread::processVoucher()
{
    _event.signal();
}

bool LockerControlThread::requestServer(int reqType)
{
    bool ret = false;
    LockerServerNetcomm* pNetcomm = LockerServerNetcomm::getInstance();
    //MessageBoxW(NULL, L"OK", L"OK", MB_OK);

    for (int counter = LockerConfig::getInstance()->servers.size(); --counter >= 0; ) {
        for (int i = 0; i < 11; ++i) {
            if (pNetcomm->makeTransaction(reqType)) {
                ret = true;
                break;
            }
            Thread::sleep(3 * 1000);
        }

        if (ret == true) {
            break;
        }

        pNetcomm->nextHost();
#ifdef _DEBUG
        Thread::sleep(3 * 1000); // 3 минуты
#else
        Thread::sleep(60 * 1000); // 11 минуты
#endif // _DEBUG

    }
    return ret;
}

void LockerControlThread::run()
{
    if (!checkVoucherStatus()) {
        while (_event.wait()) {
            if (requestServer(LockerServerNetcomm::CheckStatus)) {
                if (checkVoucherStatus()) {
                    break;
                }
            }
            else {
                Thread::sleep(3 * 60 * 1000);
               _event.signal();
            }
        }
    }

    zgui::TNotifyUI notify;
    notify.sType = "vcomplete";
    _pPaintMgr->SendNotify(notify, true);

//    ::PostQuitMessage(0L);
}

bool LockerControlThread::checkVoucherStatus()
{
    LockerConfig* pCfg = LockerConfig::getInstance();
    bool ret = false;

    if (pCfg->activeVoucherStatus == LockerConfig::NeedNodeInfo) {
        requestServer(LockerServerNetcomm::GetInfo);
        pCfg->activeVoucherStatus = LockerConfig::Uninitialized;
        _event.signal();
        return ret;
    }
    else if (pCfg->activeVoucherStatus == LockerConfig::InProgress) {
        ELockerAlertWindow::show(_hWnd, ELockerAlertWindow::NoButtonAlert);
    }

    if (pCfg->activeVoucherStatus == LockerConfig::Good) {
        ret = true;
    }
    else if (pCfg->activeVoucherStatus == LockerConfig::Invalid/* || pCfg->activeVoucherStatus == LockerConfig::Unknown*/) {
        ELockerAlertWindow::show(_hWnd, ELockerAlertWindow::OneButtonAlert, "key_error", "key_bad");
        pCfg->addVoucherAsUsed(pCfg->activeVoucher);
        pCfg->activeVoucher = zgui::String::empty;
        pCfg->activeVoucherStatus = LockerConfig::Uninitialized;
    }

    return ret;
}
