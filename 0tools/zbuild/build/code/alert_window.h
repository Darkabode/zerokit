#ifndef __ELOCKER_ALERTWINDOW_H_
#define __ELOCKER_ALERTWINDOW_H_

class ELockerAlertWindow : public zgui::Window, public zgui::INotifyUI, public zgui::Thread
{
public:
    enum AlertType {
        OneButtonAlert = 1,
        TwoButtonAlert = 2,
        NoButtonAlert = 3,
    };

    ELockerAlertWindow(int alertType, const zgui::String& title, const zgui::String& message);

    LPCTSTR GetWindowClassName() const;
    UINT GetClassStyle() const;

    void Init();
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    int getStatus() const { return _status; }

    static int show(HWND hWnd, int alertType, const zgui::String& title = zgui::String::empty, const zgui::String& message = zgui::String::empty);

private:
    void run();

//     LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void Notify(zgui::TNotifyUI& msg);

    zgui::CPaintManagerUI m_pm;

    int _alertType;
    int _status;
    zgui::CButtonUI* _pButtonOk;
    zgui::CButtonUI* _pButtonCancel;
    zgui::CLabelUI* _pTitle;
    zgui::CTextUI* _pMessage;
    zgui::String _title;
    zgui::String _message;
};

#endif // __ELOCKER_ALERTWINDOW_H_
