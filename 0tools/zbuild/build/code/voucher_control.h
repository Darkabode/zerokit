#ifndef __ELOCKER_VOUCHERCONTROL_H_
#define __ELOCKER_VOUCHERCONTROL_H_

class ELockerVoucherControl
{
public:
    enum Type {
        PSC_Voucher = 1, // Не используется
        UKASH_Voucher,
        MPAK_Voucher,
    };

    ELockerVoucherControl(zgui::CVerticalLayoutUI* pPaymentLayout, zgui::CPaintManagerUI* pPaintMgr, HWND hWnd, Type voucherType);
    ~ELockerVoucherControl();

    void handleButtonClick(zgui::TNotifyUI& msg);

protected:
    virtual bool validateVoucher() = 0;

    Type _voucherType;
    zgui::CVerticalLayoutUI* _pPaymentLayout;
    zgui::CContainerUI* _pStores;
    zgui::CTextUI* _pCode;

    zgui::CButtonUI* _buttons[13];

    zgui::CPaintManagerUI* _pPaintMgr;
    zgui::String _voucherCode;
    int _codeLength;
    zgui::String _emptyText;
    uint32_t _emptyColor;
    HWND _mainHWnd;
    
};


class ELockerMpakVoucherControl : public ELockerVoucherControl
{
public:
    ELockerMpakVoucherControl(zgui::CVerticalLayoutUI* pPaymentLayout, zgui::CPaintManagerUI* pPaintMgr, HWND hWnd);
    ~ELockerMpakVoucherControl();

private:
    bool validateVoucher();
};

class ELockerUkashVoucherControl : public ELockerVoucherControl
{
public:
    ELockerUkashVoucherControl(zgui::CVerticalLayoutUI* pPaymentLayout, zgui::CPaintManagerUI* pPaintMgr, HWND hWnd);
    ~ELockerUkashVoucherControl();

private:
    bool validateVoucher();
};

class ELockerPscVoucherControl : public ELockerVoucherControl
{
public:
    ELockerPscVoucherControl(zgui::CVerticalLayoutUI* pPaymentLayout, zgui::CPaintManagerUI* pPaintMgr, HWND hWnd);
    ~ELockerPscVoucherControl();

private:
    bool validateVoucher();
};

#endif // __ELOCKER_VOUCHERCONTROL_H_
