ELockerAlertWindow::ELockerAlertWindow(int alertType, const zgui::String& title, const zgui::String& message) :
_alertType(alertType),
_pTitle(0),
_pMessage(0),
_title(title),
_message(message)
{
}

LPCTSTR ELockerAlertWindow::GetWindowClassName() const
{
    return _T("AlertWindow");
};

UINT ELockerAlertWindow::GetClassStyle() const
{
    return UI_CLASSSTYLE_DIALOG;
};

void ELockerAlertWindow::run()
{
    LockerConfig* pCfg = LockerConfig::getInstance();

    while (pCfg->activeVoucherStatus == LockerConfig::InProgress) {
        Thread::sleep(pCfg->waitTimeout);
        LockerServerNetcomm::getInstance()->makeTransaction(LockerServerNetcomm::CheckStatus);
    }

    zgui::TNotifyUI notify;
    notify.sType = "vcomplete";
    m_pm.SendNotify(notify, true);
}

void ELockerAlertWindow::Init()
{
    int i;
    zgui::String key;

    if (_alertType < NoButtonAlert) {
        _pButtonOk = static_cast<zgui::CButtonUI*>(m_pm.FindControl(_T("b_ok")));
    }
    if (_alertType == TwoButtonAlert) {
        _pButtonCancel = static_cast<zgui::CButtonUI*>(m_pm.FindControl(_T("b_cancel")));
    }

    if (_alertType != NoButtonAlert) {
        _pTitle = static_cast<zgui::CLabelUI*>(m_pm.FindControl(_T("l_title")));
    }
    
    _pMessage = static_cast<zgui::CTextUI*>(m_pm.FindControl(_T("l_message")));

    if (_alertType != NoButtonAlert && _title.startsWith("key_")) {
        zgui::StringArray titles;
        titles.addTokens(_pTitle->GetUserData(), ";", zgui::String::empty);

        for (i = 0; i < titles.size(); ++i) {
            zgui::StringArray keyVal;

            keyVal.addTokens(titles[i], "=", zgui::String::empty);
            
            key = keyVal[0].trim();
            if (key == _title) {
                _title = zgui::String::empty;
                _title << "{f 0}" << keyVal[1].trim() << "{/f}";
                break;
            }
        }
    }

    if (_message.startsWith("key_")) {
        zgui::StringArray messages;
        messages.addTokens(_pMessage->GetUserData(), ";", zgui::String::empty);

        for (i = 0; i < messages.size(); ++i) {
            zgui::StringArray keyVal;

            keyVal.addTokens(messages[i], "=", zgui::String::empty);

            key = keyVal[0].trim();
            if (key == _message) {
                _message = zgui::String::empty;
                _message << "{f 1}" << keyVal[1].trim() << "{/f}";
                break;
            }
        }
    }
    else if (!_pMessage->GetText().isEmpty()) {
        _message = _pMessage->GetText();
    }

    if (_alertType != NoButtonAlert && _pTitle != 0) {
        _pTitle->SetText(_title);
    }

    if (_pMessage != 0) {
        _pMessage->SetText(_message);
    }

    if (_alertType == NoButtonAlert) {
        // «апускаем поток, который будет периодичеспки провер€ть статус обработки ваучера.
        startThread();
    }
}

LRESULT ELockerAlertWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
    styleValue &= ~WS_CAPTION;
    ::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    m_pm.Init(_hWnd);
    zgui::GuiBuilder builder;
    zgui::String xmlName("alert");
    xmlName << _alertType << ".xml";
    zgui::CControlUI* pRoot = builder.Create(xmlName, (UINT)0, 0, &m_pm);
    ASSERT(pRoot && "Failed to parse XML");
    m_pm.AttachDialog(pRoot);
    m_pm.AddNotifier(this);

    Init();

    return 0;
}

// LRESULT ELockerAlertWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
// {
//     bHandled = TRUE;
//     return 0;
// }

void ELockerAlertWindow::Notify(zgui::TNotifyUI& msg)
{
    if (msg.sType == "click") {
        if (msg.pSender == _pButtonOk) {
            _status = 1;
            Close();
            return;
        }
        else if (msg.pSender == _pButtonCancel) {
            _status = 0;
            Close();
            return;
        }
    }
    else if (msg.sType == "vcomplete") {
        _status = 1;
        Close();
    }
}

LRESULT ELockerAlertWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    BOOL bHandled = TRUE;
    switch (uMsg) {
            case WM_CREATE:
                lRes = OnCreate(uMsg, wParam, lParam, bHandled);
                break;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                bHandled = true;
                lRes = 0;
                break;
//             case WM_CLOSE:
//                 lRes = OnClose(uMsg, wParam, lParam, bHandled);
//                 break;
//             case WM_DESTROY:       lRes = OnDestroy(uMsg, wParam, lParam, bHandled); break;
//             case WM_NCACTIVATE:    lRes = OnNcActivate(uMsg, wParam, lParam, bHandled); break;
//             case WM_NCCALCSIZE:    lRes = OnNcCalcSize(uMsg, wParam, lParam, bHandled); break;
//             case WM_NCPAINT:       lRes = OnNcPaint(uMsg, wParam, lParam, bHandled); break;
//             case WM_NCHITTEST:     lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
//             case WM_SIZE:          lRes = OnSize(uMsg, wParam, lParam, bHandled); break;
//             case WM_GETMINMAXINFO: lRes = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled); break;
//             case WM_SYSCOMMAND:    lRes = OnSysCommand(uMsg, wParam, lParam, bHandled); break;
            default:
                bHandled = FALSE;
    }
    if (bHandled) {
        return lRes;
    }
    if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes)) {
        return lRes;
    }

    return Window::HandleMessage(uMsg, wParam, lParam);
}

int ELockerAlertWindow::show(HWND hWnd, int alertType, const zgui::String& title, const zgui::String& message)
{
    ELockerAlertWindow alertWindow(alertType, title, message);
    alertWindow.Create(hWnd, _T(""), UI_WNDSTYLE_DIALOG, 0, 0, 0, 0, 0, NULL);
    alertWindow.CenterWindow();
    alertWindow.ShowModal();

    return alertWindow.getStatus();
}
