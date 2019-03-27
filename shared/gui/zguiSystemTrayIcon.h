#ifndef __ZGUI_SYSTEMTRAYICON_H_
#define __ZGUI_SYSTEMTRAYICON_H_

namespace zgui {

template <class T>
class SystemTrayIcon
{
public:
	enum {
		TRAYICON_TIMER_ID = 0x79797979
	};
	SystemTrayIcon() :
	_installed(false),
	_nDefault(0),
	_animating(false),
	_hSavedIcon(0)
	{
		WM_TRAYICON = fn_RegisterWindowMessageW(L"WM_TRAYICON");
		__stosb((uint8_t*)&_nid, 0, sizeof(NOTIFYICONDATAW));
	}

	~SystemTrayIcon()
	{
		removeIcon();
	}

	bool installIcon(const String& tooltip, HICON hIcon)
	{
		T* pT = static_cast<T*>(this);
		_nid.cbSize = sizeof(NOTIFYICONDATAW);
		_nid.hWnd = pT->getHWND();
		_nid.uID = (UINT)_nid.hWnd;
		_nid.hIcon = hIcon;
		_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		_nid.uCallbackMessage = WM_TRAYICON;
		fn_lstrcpynW(_nid.szTip, tooltip.toWideCharPointer(), 63);
		_installed = fn_Shell_NotifyIconW(NIM_ADD, &_nid) ? true : false;
		return _installed;
	}

	bool setIcon(HICON hIcon)
	{
		_nid.hIcon = hIcon;
		_nid.uFlags = NIF_ICON;
		return (fn_Shell_NotifyIconW(NIM_MODIFY, &_nid) ? true : false);
	}

	void setIcons(Array<HICON> icons)
	{
		_icons = icons;
	}

	HICON getIcon() const
	{
		return _nid.hIcon;
	}

	void startIconAnimation(uint32_t delay)
	{
		if (_icons.size() != 0) {
			stopAnimation();
			_currentIcon = 0;

			_hSavedIcon = getIcon();

			T* pT = static_cast<T*>(this);

			if (pT->getPaintManager().SetTimer(pT->getTimerControl(), TRAYICON_TIMER_ID, delay)) {
				_animating = true;
			}
		}
	}

	void stopAnimation()
	{
		T* pT = static_cast<T*>(this);
		if (_animating) {
			if (pT->getPaintManager().KillTimer(pT->getTimerControl(), TRAYICON_TIMER_ID)) {
				_animating = false;
			}
			if (_hSavedIcon != 0) {
				setIcon(_hSavedIcon);
				_hSavedIcon = 0;
			}
		}
	}

	void stepAnimation()
	{
		if (_icons.size() != 0) {
			if (_currentIcon >= _icons.size()) {
				_currentIcon = 0;
			}
			setIcon(_icons.getUnchecked(_currentIcon));
			++_currentIcon;
		}
	}

	bool removeIcon()
	{
		if (!_installed) {
			return false;
		}
		_nid.uFlags = 0;
		return fn_Shell_NotifyIconW(NIM_DELETE, &_nid) ? true : false;
	}

	bool setTooltip(LPCTSTR pszTooltipText)
	{
		if (pszTooltipText == 0) {
			return false;
		}

		_nid.uFlags = NIF_TIP;
		fn_lstrcpynW(_nid.szTip, pszTooltipText, 63);
		return fn_Shell_NotifyIconW(NIM_MODIFY, &_nid) ? true : false;
	}

	// Set the default menu item ID
	inline void SetDefaultItem(UINT nID) {
		_nDefault = nID;
	}

protected:
	UINT WM_TRAYICON;
	NOTIFYICONDATAW _nid;
	bool _installed;
	UINT _nDefault;
	Array<HICON> _icons;
	bool _animating;
	uint32_t _currentIcon;
	HICON _hSavedIcon;
};


}

#endif // __ZGUI_SYSTEMTRAYICON_H_
