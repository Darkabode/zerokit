#ifdef SIMPLE_MODE

HHOOK hKeyboardHook = 0;  // Old low level keyboard hook 

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) 
{
//    PKBDLLHOOKSTRUCT p;

    if (nCode == HC_ACTION) {
//         p = (PKBDLLHOOKSTRUCT) lParam;
// 
//         if (
//             // WIN key (for Start Menu)
//             ((p->vkCode == VK_LWIN) || (p->vkCode == VK_RWIN)) ||       
//             // ALT+TAB
//             (p->vkCode == VK_TAB && p->flags & LLKHF_ALTDOWN) ||       
//             // ALT+ESC
//             (p->vkCode == VK_ESCAPE && p->flags & LLKHF_ALTDOWN) ||    
//             // CTRL+ESC
//             ((p->vkCode == VK_ESCAPE) && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0)) ||
//             // CTRL+SHIFT+ESC
//             ((p->vkCode == VK_ESCAPE) && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) && ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)) ||
//             // CTRL+ALT+DEL (Unfortunately doesn't work !)
//             ((p->vkCode == VK_DELETE) && ( (p->flags & LLKHF_ALTDOWN) != 0 ) && ( (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0))
//             ) {
                return 1;
//         }
    }

    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

bool locker_keyboard_enable_disable(bool bEnableDisable)
{
    bool ret = true;

    if (!bEnableDisable) {
        if (!hKeyboardHook) {
            // Install the low-level keyboard hook
            hKeyboardHook  = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(0), 0);
            if (hKeyboardHook == 0) {
                ret = false;
            }
        }
    }
    else {
        UnhookWindowsHookEx(hKeyboardHook);
        hKeyboardHook = 0;
    }

    return ret;
}

#endif // SIMPLE_MODE