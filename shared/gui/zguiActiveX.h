#ifndef __ZGUI_ACTIVEX_H_
#define __ZGUI_ACTIVEX_H_

#ifdef ZGUI_USE_ACTIVEX
#include <mshtmhst.h>

struct IOleObject;

namespace zgui {

class ActiveXCtrl;

template< class T >
class CSafeRelease
{
public:
    CSafeRelease(T* p) :
	m_p(p)
	{
	};
    ~CSafeRelease()
	{
		if (m_p != 0) {
			m_p->Release();
		}
	};

	T* Detach()
	{
		T* t = m_p;
		m_p = NULL;
		return t;
	};
    T* m_p;
};

class ActiveX : public Control, public IMessageFilterUI
{
    friend class ActiveXCtrl;
public:
    ActiveX();
    virtual ~ActiveX();

    const String& getClass() const;
	LPVOID getInterface(const String& name);

    HWND GetHostWindow() const;

    bool IsDelayCreate() const;
    void SetDelayCreate(bool bDelayCreate = true);

    bool CreateControl(const CLSID clsid);
    bool CreateControl(const String& pstrCLSID);
    HRESULT GetControl(const IID iid, LPVOID* ppRet);
	CLSID GetClisd() const;
    const String& GetModuleName() const;
    void SetModuleName(const String& pstrText);

    void SetVisible(bool bVisible = true);
    void SetInternVisible(bool bVisible = true);
    void SetPos(RECT rc);
    void DoPaint(HDC hDC, const RECT& rcPaint);

	void setAttribute(const String& pstrName, const String& pstrValue);

    LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);

protected:
    virtual void ReleaseControl();
    virtual bool DoCreateControl();

protected:
    CLSID m_clsid;
    String m_sModuleName;
    bool m_bCreated;
    bool m_bDelayCreate;
    IOleObject* m_pUnk;
    ActiveXCtrl* m_pControl;
    HWND m_hwndHost;
};

} // namespace DuiLib

#endif // ZGUI_USE_ACTIVEX

#endif // __ZGUI_ACTIVEX_H_
