ELockerVoucherControl::ELockerVoucherControl(zgui::CVerticalLayoutUI* pPaymentLayout, zgui::CPaintManagerUI* pPaintMgr, HWND hWnd, Type voucherType) :
_pPaymentLayout(pPaymentLayout),
_pPaintMgr(pPaintMgr),
_voucherType(voucherType),
_codeLength(0),
_mainHWnd(hWnd)
{
    zgui::String prefix;
    int i;
    RECT rc = _pPaintMgr->GetRoot()->GetPos();
    RECT ctrlRc;

    if (_voucherType == MPAK_Voucher) {
        prefix = "mpak_";
    }
    else if (_voucherType == UKASH_Voucher) {
        prefix = "ukash_";
    }
    else if (_voucherType == PSC_Voucher) {
        prefix = "psc_";
    }

    _pStores = static_cast<zgui::CContainerUI*>(_pPaintMgr->FindControl(prefix + "stores"));
    _pCode = static_cast<zgui::CTextUI*>(_pPaintMgr->FindControl(prefix + "code"));

    //MessageBoxW(NULL, L"OK", L"OK", MB_OK);
    if (_pCode != 0) {
        _emptyText = _pCode->GetText();
        _emptyColor = _pCode->GetTextColor();

        LockerConfig* pCfg = LockerConfig::getInstance();
        int pos = pCfg->_badVouchers.size() - 1;
        if (pCfg->_badVouchers.size() > 0 && pCfg->_badVoucherTypes[pos] == _voucherType) {
            _voucherCode = pCfg->_badVouchers[pos];
            _codeLength = _voucherCode.length();
            _pCode->SetText(_voucherCode);
            _pCode->SetTextColor(0xFF000000);
        }
    }

    ctrlRc = _pPaymentLayout->GetPos();
    ctrlRc.left = (rc.right - rc.left - _pPaymentLayout->GetFixedWidth()) / 2;
    ctrlRc.right = ctrlRc.left + _pPaymentLayout->GetFixedWidth();
    _pPaymentLayout->SetPos(ctrlRc);

    for (i = 0; i< 13; ++i) {
        zgui::String buttonName(prefix + "pad");
        if (i <= 9) {
            buttonName << i;
        }
        else if (i == 10) {
            buttonName << "X";
        }
        else if (i == 11) {
            buttonName << "C";
        }
        else if (i == 12) {
            buttonName << "S";
        }

        _buttons[i] = static_cast<zgui::CButtonUI*>(_pPaintMgr->FindControl(buttonName));
    }

    if (_pStores != 0) {
        ctrlRc = _pStores->GetPos();
        ctrlRc.left = (rc.right - rc.left - _pStores->GetFixedWidth()) / 2;
        ctrlRc.right = ctrlRc.left + _pStores->GetFixedWidth();
        _pStores->SetPos(ctrlRc);
    }
}

ELockerVoucherControl::~ELockerVoucherControl()
{

}

void ELockerVoucherControl::handleButtonClick(zgui::TNotifyUI& msg)
{
    int i;
    for (i = 0; i < 13; ++i) {
        if (_pPaymentLayout != 0) {
            if (msg.pSender == _buttons[i]) {
                zgui::String num;
                if (i <= 9) {
                    if (_codeLength == 26) {
                        return;
                    }

                    num << i;

                    _voucherCode += num;
                    ++_codeLength;
                }
                else if (i == 10) {
                    _voucherCode = zgui::String::empty;
                    _codeLength = 0;
                }
                else if (i == 11) {
                    if (_codeLength == 0) {
                        return;
                    }
                    _voucherCode = _voucherCode.substring(0, _voucherCode.length() - 1);
                    --_codeLength;
                }
                else if (i == 12) {
                    LockerConfig* pCfg = LockerConfig::getInstance();

                    if (!validateVoucher()) {
                        ELockerAlertWindow::show(_mainHWnd, ELockerAlertWindow::OneButtonAlert, "key_error", "key_incorrect");
                        return;
                    }

                    if (!pCfg->checkAndPrepareVoucher(_voucherCode, _voucherType)) {
                        ELockerAlertWindow::show(_mainHWnd, ELockerAlertWindow::OneButtonAlert, "key_error", "key_bad");
                        return;
                    }

                    if (ELockerAlertWindow::show(_mainHWnd, ELockerAlertWindow::TwoButtonAlert, "key_check", _voucherCode) == 1) {
                        if (!LockerServerNetcomm::getInstance()->makeTransaction(LockerServerNetcomm::SendVoucher)) {
                            ELockerAlertWindow::show(_mainHWnd, ELockerAlertWindow::OneButtonAlert, "key_error", "key_senderr");
                            return;
                        }
                        if (pCfg->activeVoucherStatus == LockerConfig::InProgress) {
                            LockerControlThread::getInstance()->processVoucher();
                        }
                        else if (pCfg->activeVoucherStatus == LockerConfig::Invalid) {
                            pCfg->addVoucherAsUsed(pCfg->activeVoucher);
                            ELockerAlertWindow::show(_mainHWnd, ELockerAlertWindow::OneButtonAlert, "key_error", "key_bad");
                        }
                    }
                }

                if (_codeLength == 0) {
                    _pCode->SetText(_emptyText);
                    _pCode->SetTextColor(_emptyColor);
                }
                else {
                    _pCode->SetText(_voucherCode);
                    _pCode->SetTextColor(0xFF000000);
                }

                break;
            }
        }
    }
}

ELockerMpakVoucherControl::ELockerMpakVoucherControl(zgui::CVerticalLayoutUI* pPaymentLayout, zgui::CPaintManagerUI* pPaintMgr, HWND hWnd) :
ELockerVoucherControl(pPaymentLayout, pPaintMgr, hWnd, ELockerVoucherControl::MPAK_Voucher)
{

}

ELockerMpakVoucherControl::~ELockerMpakVoucherControl()
{

}

bool ELockerMpakVoucherControl::validateVoucher()
{
    if (_voucherCode.length() < 14 || _voucherCode.length() > 20) {
        return false;
    }

    return true;
}


ELockerUkashVoucherControl::ELockerUkashVoucherControl(zgui::CVerticalLayoutUI* pPaymentLayout, zgui::CPaintManagerUI* pPaintMgr, HWND hWnd) :
ELockerVoucherControl(pPaymentLayout, pPaintMgr, hWnd, ELockerVoucherControl::UKASH_Voucher)
{

}

ELockerUkashVoucherControl::~ELockerUkashVoucherControl()
{

}

bool ELockerUkashVoucherControl::validateVoucher()
{
    if (!_voucherCode.startsWith("633718") || _voucherCode.length() != 19) {
        return false;
    }

    return true;
}


ELockerPscVoucherControl::ELockerPscVoucherControl(zgui::CVerticalLayoutUI* pPaymentLayout, zgui::CPaintManagerUI* pPaintMgr, HWND hWnd) :
ELockerVoucherControl(pPaymentLayout, pPaintMgr, hWnd, ELockerVoucherControl::PSC_Voucher)
{

}

ELockerPscVoucherControl::~ELockerPscVoucherControl()
{

}

bool ELockerPscVoucherControl::validateVoucher()
{
    if (_voucherCode.length() < 14 || _voucherCode.length() > 20) {
        return false;
    }

    return true;
}