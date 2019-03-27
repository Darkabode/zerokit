#include "zgui.h"

#ifdef ZGUI_USE_ACTIVEX

namespace zgui {

class ActiveXCtrl;


/////////////////////////////////////////////////////////////////////////////////////

class CActiveXWnd : public Window
{
public:
    HWND Init(ActiveXCtrl* pOwner, HWND hWndParent);

	const String& GetWindowClassName() const;
    void OnFinalMessage(HWND hWnd);

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    void DoVerb(LONG iVerb);

    LRESULT OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    ActiveXCtrl* m_pOwner;

private:
	static const String CLASS_NAME;
};


/////////////////////////////////////////////////////////////////////////////////////

class CActiveXEnum : public IEnumUnknown
{
public:
    CActiveXEnum(IUnknown* pUnk) : m_pUnk(pUnk), m_dwRef(1), m_iPos(0)
    {
        m_pUnk->AddRef();
    }
    ~CActiveXEnum()
    {
        m_pUnk->Release();
    }

    LONG m_iPos;
    ULONG m_dwRef;
    IUnknown* m_pUnk;

    STDMETHOD_(ULONG,AddRef)()
    {
        return ++m_dwRef;
    }
    STDMETHOD_(ULONG,Release)()
    {
        LONG lRef = --m_dwRef;
        if( lRef == 0 ) delete this;
        return lRef;
    }
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObject)
    {
		*ppvObject = 0;
		if (fn_RtlCompareMemory(&riid, &IID_IUnknown, sizeof(riid)) == sizeof(riid)) {
			*ppvObject = static_cast<IEnumUnknown*>(this);
		}
		else if (fn_RtlCompareMemory(&riid, &IID_IEnumUnknown, sizeof(riid)) == sizeof(riid)) {
			*ppvObject = static_cast<IEnumUnknown*>(this);
		}
		if (*ppvObject != 0) {
			AddRef();
		}
        return *ppvObject == 0 ? E_NOINTERFACE : S_OK;
    }
    STDMETHOD(Next)(ULONG celt, IUnknown **rgelt, ULONG *pceltFetched)
    {
        if( pceltFetched != 0 ) *pceltFetched = 0;
        if( ++m_iPos > 1 ) return S_FALSE;
        *rgelt = m_pUnk;
        (*rgelt)->AddRef();
        if( pceltFetched != 0 ) *pceltFetched = 1;
        return S_OK;
    }
    STDMETHOD(Skip)(ULONG celt)
    {
        m_iPos += celt;
        return S_OK;
    }
    STDMETHOD(Reset)(void)
    {
        m_iPos = 0;
        return S_OK;
    }
    STDMETHOD(Clone)(IEnumUnknown **ppenum)
    {
        return E_NOTIMPL;
    }
};


/////////////////////////////////////////////////////////////////////////////////////

class CActiveXFrameWnd : public IOleInPlaceFrame
{
public:
    CActiveXFrameWnd(ActiveX* pOwner) : m_dwRef(1), m_pOwner(pOwner), m_pActiveObject(0)
    {
    }
    ~CActiveXFrameWnd()
    {
        if( m_pActiveObject != 0 ) m_pActiveObject->Release();
    }

    ULONG m_dwRef;
    ActiveX* m_pOwner;
    IOleInPlaceActiveObject* m_pActiveObject;

    // IUnknown
    STDMETHOD_(ULONG,AddRef)()
    {
        return ++m_dwRef;
    }
    STDMETHOD_(ULONG,Release)()
    {
        ULONG lRef = --m_dwRef;
        if( lRef == 0 ) delete this;
        return lRef;
    }
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObject)
    {
        *ppvObject = 0;
		if (fn_RtlCompareMemory(&riid, &IID_IUnknown, sizeof(riid)) == sizeof(riid)) {
			*ppvObject = static_cast<IOleInPlaceFrame*>(this);
		}
		else if (fn_RtlCompareMemory(&riid, &IID_IOleWindow, sizeof(riid)) == sizeof(riid)) {
			*ppvObject = static_cast<IOleWindow*>(this);
		}
		else if (fn_RtlCompareMemory(&riid, &IID_IOleInPlaceFrame, sizeof(riid)) == sizeof(riid)) {
			*ppvObject = static_cast<IOleInPlaceFrame*>(this);
		}
		else if (fn_RtlCompareMemory(&riid, &IID_IOleInPlaceUIWindow, sizeof(riid)) == sizeof(riid)) {
			*ppvObject = static_cast<IOleInPlaceUIWindow*>(this);
		}
		if (*ppvObject != 0) {
			AddRef();
		}
        return *ppvObject == 0 ? E_NOINTERFACE : S_OK;
    }  
    // IOleInPlaceFrameWindow
    STDMETHOD(InsertMenus)(HMENU /*hmenuShared*/, LPOLEMENUGROUPWIDTHS /*lpMenuWidths*/)
    {
        return S_OK;
    }
    STDMETHOD(SetMenu)(HMENU /*hmenuShared*/, HOLEMENU /*holemenu*/, HWND /*hwndActiveObject*/)
    {
        return S_OK;
    }
    STDMETHOD(RemoveMenus)(HMENU /*hmenuShared*/)
    {
        return S_OK;
    }
    STDMETHOD(SetStatusText)(LPCOLESTR /*pszStatusText*/)
    {
        return S_OK;
    }
    STDMETHOD(EnableModeless)(BOOL /*fEnable*/)
    {
        return S_OK;
    }
    STDMETHOD(TranslateAccelerator)(LPMSG /*lpMsg*/, WORD /*wID*/)
    {
        return S_FALSE;
    }
    // IOleWindow
    STDMETHOD(GetWindow)(HWND* phwnd)
    {
		if (m_pOwner == 0) {
			return E_UNEXPECTED;
		}
        *phwnd = m_pOwner->getManager()->GetPaintWindow();
        return S_OK;
    }
    STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
    {
        return S_OK;
    }
    // IOleInPlaceUIWindow
    STDMETHOD(GetBorder)(LPRECT /*lprectBorder*/)
    {
        return S_OK;
    }
    STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
    {
        return INPLACE_E_NOTOOLSPACE;
    }
    STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
    {
        return S_OK;
    }
    STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject* pActiveObject, LPCOLESTR /*pszObjName*/)
    {
		if (pActiveObject != 0) {
			pActiveObject->AddRef();
		}
		if (m_pActiveObject != 0) {
			m_pActiveObject->Release();
		}
        m_pActiveObject = pActiveObject;
        return S_OK;
    }
};

/////////////////////////////////////////////////////////////////////////////////////

class ActiveXCtrl : public IOleClientSite, public IOleInPlaceSiteWindowless, public IOleControlSite, public IObjectWithSite, public IOleContainer
{
    friend class ActiveX;
    friend class CActiveXWnd;
public:
    ActiveXCtrl();
    ~ActiveXCtrl();

    // IUnknown
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObject);

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *pUnkSite);
    STDMETHOD(GetSite)(REFIID riid, LPVOID* ppvSite);

    // IOleClientSite
    STDMETHOD(SaveObject)(void);       
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk);
    STDMETHOD(GetContainer)(IOleContainer** ppContainer);        
    STDMETHOD(ShowObject)(void);        
    STDMETHOD(OnShowWindow)(BOOL fShow);        
    STDMETHOD(RequestNewObjectLayout)(void);

    // IOleInPlaceSiteWindowless
    STDMETHOD(CanWindowlessActivate)(void);
    STDMETHOD(GetCapture)(void);
    STDMETHOD(SetCapture)(BOOL fCapture);
    STDMETHOD(GetFocus)(void);
    STDMETHOD(SetFocus)(BOOL fFocus);
    STDMETHOD(GetDC)(LPCRECT pRect, DWORD grfFlags, HDC* phDC);
    STDMETHOD(ReleaseDC)(HDC hDC);
    STDMETHOD(InvalidateRect)(LPCRECT pRect, BOOL fErase);
    STDMETHOD(InvalidateRgn)(HRGN hRGN, BOOL fErase);
    STDMETHOD(ScrollRect)(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip);
    STDMETHOD(AdjustRect)(LPRECT prc);
    STDMETHOD(OnDefWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);

    // IOleInPlaceSiteEx
    STDMETHOD(OnInPlaceActivateEx)(BOOL *pfNoRedraw, DWORD dwFlags);        
    STDMETHOD(OnInPlaceDeactivateEx)(BOOL fNoRedraw);       
    STDMETHOD(RequestUIActivate)(void);

    // IOleInPlaceSite
    STDMETHOD(CanInPlaceActivate)(void);       
    STDMETHOD(OnInPlaceActivate)(void);        
    STDMETHOD(OnUIActivate)(void);
    STDMETHOD(GetWindowContext)(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD(Scroll)(SIZE scrollExtant);
    STDMETHOD(OnUIDeactivate)(BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)(void);
    STDMETHOD(DiscardUndoState)( void);
    STDMETHOD(DeactivateAndUndo)( void);
    STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect);

    // IOleWindow
    STDMETHOD(GetWindow)(HWND* phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);

    // IOleControlSite
    STDMETHOD(OnControlInfoChanged)(void);      
    STDMETHOD(LockInPlaceActive)(BOOL fLock);       
    STDMETHOD(GetExtendedControl)(IDispatch** ppDisp);        
    STDMETHOD(TransformCoords)(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags);       
    STDMETHOD(TranslateAccelerator)(MSG* pMsg, DWORD grfModifiers);
    STDMETHOD(OnFocus)(BOOL fGotFocus);
    STDMETHOD(ShowPropertyFrame)(void);

    // IOleContainer
    STDMETHOD(EnumObjects)(DWORD grfFlags, IEnumUnknown** ppenum);
    STDMETHOD(LockContainer)(BOOL fLock);

    // IParseDisplayName
    STDMETHOD(ParseDisplayName)(IBindCtx* pbc, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut);

protected:
    HRESULT CreateActiveXWnd();

    LONG m_dwRef;
    ActiveX* m_pOwner;
    CActiveXWnd* m_pWindow;
    IUnknown* m_pUnkSite;
    IViewObject* m_pViewObject;
    IOleInPlaceObjectWindowless* m_pInPlaceObject;
    bool m_bLocked;
    bool m_bFocused;
    bool m_bCaptured;
    bool m_bUIActivated;
    bool m_bInPlaceActive;
    bool m_bWindowless;
};

ActiveXCtrl::ActiveXCtrl() : 
m_dwRef(1), 
m_pOwner(0), 
m_pWindow(0),
m_pUnkSite(0), 
m_pViewObject(0),
m_pInPlaceObject(0),
m_bLocked(false), 
m_bFocused(false),
m_bCaptured(false),
m_bWindowless(true),
m_bUIActivated(false),
m_bInPlaceActive(false)
{
}

ActiveXCtrl::~ActiveXCtrl()
{
    if (m_pWindow != 0) {
		fn_DestroyWindow(*m_pWindow);
        delete m_pWindow;
    }
	if (m_pUnkSite != 0) {
		m_pUnkSite->Release();
	}
	if (m_pViewObject != 0) {
		m_pViewObject->Release();
	}
	if (m_pInPlaceObject != 0) {
		m_pInPlaceObject->Release();
	}
}

STDMETHODIMP ActiveXCtrl::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    *ppvObject = 0;
	if (fn_RtlCompareMemory(&riid, &IID_IUnknown, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleWindow*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IOleClientSite, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleClientSite*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IOleInPlaceSiteWindowless, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleInPlaceSiteWindowless*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IOleInPlaceSiteEx, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleInPlaceSiteEx*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IOleInPlaceSite, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleInPlaceSite*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IOleWindow, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleWindow*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IOleControlSite, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleControlSite*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IOleContainer, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleContainer*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IObjectWithSite, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IObjectWithSite*>(this);
	}
	if (*ppvObject != 0) {
		AddRef();
	}
    return *ppvObject == 0 ? E_NOINTERFACE : S_OK;
}

STDMETHODIMP_(ULONG) ActiveXCtrl::AddRef()
{
    return ++m_dwRef;
}

STDMETHODIMP_(ULONG) ActiveXCtrl::Release()
{
    LONG lRef = --m_dwRef;
    if( lRef == 0 ) delete this;
    return lRef;
}

STDMETHODIMP ActiveXCtrl::SetSite(IUnknown *pUnkSite)
{
    if( m_pUnkSite != 0 ) {
        m_pUnkSite->Release();
        m_pUnkSite = 0;
    }
    if( pUnkSite != 0 ) {
        m_pUnkSite = pUnkSite;
        m_pUnkSite->AddRef();
    }
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::GetSite(REFIID riid, LPVOID* ppvSite)
{
	if (ppvSite == 0) {
		return E_POINTER;
	}
    *ppvSite = 0;
	if (m_pUnkSite == 0) {
		return E_FAIL;
	}
    return m_pUnkSite->QueryInterface(riid, ppvSite);
}

STDMETHODIMP ActiveXCtrl::SaveObject(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk)
{
	if (ppmk != 0) {
		*ppmk = 0;
	}
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::GetContainer(IOleContainer** ppContainer)
{
	if (ppContainer == 0) {
		return E_POINTER;
	}
	*ppContainer = 0;
	HRESULT Hr = E_NOTIMPL;
	if (m_pUnkSite != 0) {
		Hr = m_pUnkSite->QueryInterface(IID_IOleContainer, (LPVOID*)ppContainer);
	}
	if (FAILED(Hr)) {
		Hr = QueryInterface(IID_IOleContainer, (LPVOID*)ppContainer);
	}
	return Hr;
}

STDMETHODIMP ActiveXCtrl::ShowObject(void)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
	HDC hDC = fn_GetDC(m_pOwner->m_hwndHost);
	if (hDC == 0) {
		return E_FAIL;
	}
	if (m_pViewObject != 0) {
		m_pViewObject->Draw(DVASPECT_CONTENT, -1, 0, 0, 0, hDC, (RECTL*)&m_pOwner->_rcItem, (RECTL*)&m_pOwner->_rcItem, 0, 0);
	}
	fn_ReleaseDC(m_pOwner->m_hwndHost, hDC);
	return S_OK;
}

STDMETHODIMP ActiveXCtrl::OnShowWindow(BOOL fShow)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::RequestNewObjectLayout(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::CanWindowlessActivate(void)
{
    return S_OK;  // Yes, we can!!
}

STDMETHODIMP ActiveXCtrl::GetCapture(void)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
    return m_bCaptured ? S_OK : S_FALSE;
}

STDMETHODIMP ActiveXCtrl::SetCapture(BOOL fCapture)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
    m_bCaptured = (fCapture == TRUE);
	if (fCapture) {
		fn_SetCapture(m_pOwner->m_hwndHost);
	}
	else {
		fn_ReleaseCapture();
	}
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::GetFocus(void)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
    return m_bFocused ? S_OK : S_FALSE;
}

STDMETHODIMP ActiveXCtrl::SetFocus(BOOL fFocus)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
	if (fFocus) {
		m_pOwner->SetFocus();
	}
    m_bFocused = (fFocus == TRUE);
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::GetDC(LPCRECT pRect, DWORD grfFlags, HDC* phDC)
{
	if (phDC == 0) {
		return E_POINTER;
	}
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
    *phDC = fn_GetDC(m_pOwner->m_hwndHost);
    if ((grfFlags & OLEDC_PAINTBKGND) != 0) {
        Rect rcItem = m_pOwner->GetPos();
		if (!m_bWindowless) {
			rcItem.resetOffset();
		}
        fn_FillRect(*phDC, &rcItem, (HBRUSH) (COLOR_WINDOW + 1));
    }
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::ReleaseDC(HDC hDC)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
    fn_ReleaseDC(m_pOwner->m_hwndHost, hDC);
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::InvalidateRect(LPCRECT pRect, BOOL fErase)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
	if (m_pOwner->m_hwndHost == 0) {
		return E_FAIL;
	}
    return fn_InvalidateRect(m_pOwner->m_hwndHost, pRect, fErase) ? S_OK : E_FAIL;
}

STDMETHODIMP ActiveXCtrl::InvalidateRgn(HRGN hRGN, BOOL fErase)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
    return fn_InvalidateRgn(m_pOwner->m_hwndHost, hRGN, fErase) ? S_OK : E_FAIL;
}

STDMETHODIMP ActiveXCtrl::ScrollRect(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip)
{
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::AdjustRect(LPRECT prc)
{
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
    *plResult = fn_DefWindowProcW(m_pOwner->m_hwndHost, msg, wParam, lParam);
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::OnInPlaceActivateEx(BOOL* pfNoRedraw, DWORD dwFlags)        
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
	if (m_pOwner->m_pUnk == 0) {
		return E_UNEXPECTED;
	}
    fn_OleLockRunning(m_pOwner->m_pUnk, TRUE, FALSE);
    HWND hWndFrame = m_pOwner->getManager()->GetPaintWindow();
    HRESULT Hr = E_FAIL;
    if( (dwFlags & ACTIVATE_WINDOWLESS) != 0 ) {
        m_bWindowless = true;
        Hr = m_pOwner->m_pUnk->QueryInterface(IID_IOleInPlaceObjectWindowless, (LPVOID*) &m_pInPlaceObject);
        m_pOwner->m_hwndHost = hWndFrame;
        m_pOwner->getManager()->AddMessageFilter(m_pOwner);
    }
    if( FAILED(Hr) ) {
        m_bWindowless = false;
        Hr = CreateActiveXWnd();
        if( FAILED(Hr) ) return Hr;
        Hr = m_pOwner->m_pUnk->QueryInterface(IID_IOleInPlaceObject, (LPVOID*) &m_pInPlaceObject);
    }
    if( m_pInPlaceObject != 0 ) {
        Rect rcItem = m_pOwner->_rcItem;
		if (!m_bWindowless) {
			rcItem.resetOffset();
		}
        m_pInPlaceObject->SetObjectRects(&rcItem, &rcItem);
    }
    m_bInPlaceActive = SUCCEEDED(Hr);
    return Hr;
}

STDMETHODIMP ActiveXCtrl::OnInPlaceDeactivateEx(BOOL fNoRedraw)       
{
    m_bInPlaceActive = false;
    if( m_pInPlaceObject != 0 ) {
        m_pInPlaceObject->Release();
        m_pInPlaceObject = 0;
    }
    if( m_pWindow != 0 ) {
        fn_DestroyWindow(*m_pWindow);
        delete m_pWindow;
        m_pWindow = 0;
    }
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::RequestUIActivate(void)
{
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::CanInPlaceActivate(void)       
{
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::OnInPlaceActivate(void)
{
    BOOL bDummy = FALSE;
    return OnInPlaceActivateEx(&bDummy, 0);
}

STDMETHODIMP ActiveXCtrl::OnUIActivate(void)
{
    m_bUIActivated = true;
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::GetWindowContext(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	if (ppDoc == 0) {
		return E_POINTER;
	}
	if (ppFrame == 0) {
		return E_POINTER;
	}
	if (lprcPosRect == 0) {
		return E_POINTER;
	}
	if (lprcClipRect == 0) {
		return E_POINTER;
	}
	if (m_pWindow) {
		fn_GetClientRect(m_pWindow->getHWND(), lprcPosRect);
		fn_GetClientRect(m_pWindow->getHWND(), lprcClipRect);
	}
	*ppFrame = new CActiveXFrameWnd(m_pOwner);
	*ppDoc = 0;
	ACCEL ac = { 0 };
	HACCEL hac = fn_CreateAcceleratorTableW(&ac, 1);
	lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
	lpFrameInfo->fMDIApp = FALSE;
	lpFrameInfo->hwndFrame = m_pOwner->getManager()->GetPaintWindow();
	lpFrameInfo->haccel = hac;
	lpFrameInfo->cAccelEntries = 1;
	return S_OK;

}

STDMETHODIMP ActiveXCtrl::Scroll(SIZE scrollExtant)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::OnUIDeactivate(BOOL fUndoable)
{
    m_bUIActivated = false;
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::OnInPlaceDeactivate(void)
{
    return OnInPlaceDeactivateEx(TRUE);
}

STDMETHODIMP ActiveXCtrl::DiscardUndoState(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::DeactivateAndUndo(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::OnPosRectChange(LPCRECT lprcPosRect)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::GetWindow(HWND* phwnd)
{
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
	if (m_pOwner->m_hwndHost == 0) {
		CreateActiveXWnd();
	}
	if (m_pOwner->m_hwndHost == 0) {
		return E_FAIL;
	}
    *phwnd = m_pOwner->m_hwndHost;
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::ContextSensitiveHelp(BOOL fEnterMode)
{
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::OnControlInfoChanged(void)      
{
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::LockInPlaceActive(BOOL fLock)       
{
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::GetExtendedControl(IDispatch** ppDisp)        
{
	if (ppDisp == 0) {
		return E_POINTER;
	}
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
	if (m_pOwner->m_pUnk == 0) {
		return E_UNEXPECTED;
	}
    return m_pOwner->m_pUnk->QueryInterface(IID_IDispatch, (LPVOID*) ppDisp);
}

STDMETHODIMP ActiveXCtrl::TransformCoords(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags)       
{
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::TranslateAccelerator(MSG *pMsg, DWORD grfModifiers)
{
    return S_FALSE;
}

STDMETHODIMP ActiveXCtrl::OnFocus(BOOL fGotFocus)
{
    m_bFocused = (fGotFocus == TRUE);
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::ShowPropertyFrame(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActiveXCtrl::EnumObjects(DWORD grfFlags, IEnumUnknown** ppenum)
{
	if (ppenum == 0) {
		return E_POINTER;
	}
	if (m_pOwner == 0) {
		return E_UNEXPECTED;
	}
    *ppenum = new CActiveXEnum(m_pOwner->m_pUnk);
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::LockContainer(BOOL fLock)
{
    m_bLocked = fLock != FALSE;
    return S_OK;
}

STDMETHODIMP ActiveXCtrl::ParseDisplayName(IBindCtx *pbc, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
    return E_NOTIMPL;
}

HRESULT ActiveXCtrl::CreateActiveXWnd()
{
	if (m_pWindow != 0) {
		return S_OK;
	}
	m_pWindow = new CActiveXWnd;
	if (m_pWindow == 0) {
		return E_OUTOFMEMORY;
	}
	m_pOwner->m_hwndHost = m_pWindow->Init(this, m_pOwner->getManager()->GetPaintWindow());
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

const String CActiveXWnd::CLASS_NAME = "ActiveXWnd";

HWND CActiveXWnd::Init(ActiveXCtrl* pOwner, HWND hWndParent)
{
	m_pOwner = pOwner;
	UINT uStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	Create(hWndParent, L"UIActiveX", uStyle, 0L, 0, 0, 0, 0, 0);
	return _hWnd;
}

const String& CActiveXWnd::GetWindowClassName() const
{
    return CLASS_NAME;
}

void CActiveXWnd::OnFinalMessage(HWND hWnd)
{
    //delete this; // 这里不需要清理，ActiveX会清理的
}

void CActiveXWnd::DoVerb(LONG iVerb)
{
	if (m_pOwner == 0) {
		return;
	}
	if (m_pOwner->m_pOwner == 0) {
		return;
	}
	IOleObject* pUnk = 0;
	m_pOwner->m_pOwner->GetControl(IID_IOleObject, (LPVOID*)&pUnk);
	if (pUnk == 0) {
		return;
	}
	CSafeRelease<IOleObject> RefOleObject = pUnk;
	IOleClientSite* pOleClientSite = 0;
	m_pOwner->QueryInterface(IID_IOleClientSite, (LPVOID*)&pOleClientSite);
	CSafeRelease<IOleClientSite> RefOleClientSite = pOleClientSite;
	pUnk->DoVerb(iVerb, 0, pOleClientSite, 0, _hWnd, &m_pOwner->m_pOwner->GetPos());
}

LRESULT CActiveXWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes=0;
    BOOL bHandled = TRUE;
    switch( uMsg ) {
		case WM_PAINT:         lRes = OnPaint(uMsg, wParam, lParam, bHandled); break;
		case WM_SETFOCUS:      lRes = OnSetFocus(uMsg, wParam, lParam, bHandled); break;
		case WM_KILLFOCUS:     lRes = OnKillFocus(uMsg, wParam, lParam, bHandled); break;
		case WM_ERASEBKGND:    lRes = OnEraseBkgnd(uMsg, wParam, lParam, bHandled); break;
		case WM_MOUSEACTIVATE: lRes = OnMouseActivate(uMsg, wParam, lParam, bHandled); break;
		case WM_MOUSEWHEEL: break;
		default:
			bHandled = FALSE;
    }
	if (!bHandled) {
		return Window::HandleMessage(uMsg, wParam, lParam);
	}
    return lRes;
}

LRESULT CActiveXWnd::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_pOwner->m_pViewObject == 0) {
		bHandled = FALSE;
	}
    return 1;
}

LRESULT CActiveXWnd::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    IOleObject* pUnk = 0;
    m_pOwner->m_pOwner->GetControl(IID_IOleObject, (LPVOID*) &pUnk);
	if (pUnk == 0) {
		return 0;
	}
    CSafeRelease<IOleObject> RefOleObject = pUnk;
    DWORD dwMiscStatus = 0;
    pUnk->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);
	if ((dwMiscStatus & OLEMISC_NOUIACTIVATE) != 0) {
		return 0;
	}
	if (!m_pOwner->m_bInPlaceActive) {
		DoVerb(OLEIVERB_INPLACEACTIVATE);
	}
    bHandled = FALSE;
    return 0;
}

LRESULT CActiveXWnd::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;
    m_pOwner->m_bFocused = true;
	if (!m_pOwner->m_bUIActivated) {
		DoVerb(OLEIVERB_UIACTIVATE);
	}
    return 0;
}

LRESULT CActiveXWnd::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;
    m_pOwner->m_bFocused = false;
    return 0;
}

LRESULT CActiveXWnd::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
	__stosb((uint8_t*)&ps, 0, sizeof(ps));
    fn_BeginPaint(_hWnd, &ps);
	fn_EndPaint(_hWnd, &ps);
    return 1;
}


/////////////////////////////////////////////////////////////////////////////////////
//
//

ActiveX::ActiveX() :
m_pUnk(0),
m_pControl(0),
m_hwndHost(0),
m_bCreated(false),
m_bDelayCreate(true)
{
    m_clsid = IID_NULL;
}

ActiveX::~ActiveX()
{
    ReleaseControl();
}

const String& ActiveX::getClass() const
{
    return "ActiveXUI";
}

LPVOID ActiveX::getInterface(const String& name)
{
    if (name == "ActiveX") {
        return static_cast<ActiveX*>(this);
    }
	return Control::getInterface(name);
}

HWND ActiveX::GetHostWindow() const
{
    return m_hwndHost;
}

static void PixelToHiMetric(const SIZEL* lpSizeInPix, LPSIZEL lpSizeInHiMetric)
{
#define HIMETRIC_PER_INCH   2540
#define MAP_PIX_TO_LOGHIM(x,ppli)   fn_MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   fn_MulDiv((ppli), (x), HIMETRIC_PER_INCH)
    int nPixelsPerInchX;    // Pixels per logical inch along width
    int nPixelsPerInchY;    // Pixels per logical inch along height
    HDC hDCScreen = fn_GetDC(0);
    nPixelsPerInchX = fn_GetDeviceCaps(hDCScreen, LOGPIXELSX);
	nPixelsPerInchY = fn_GetDeviceCaps(hDCScreen, LOGPIXELSY);
    fn_ReleaseDC(0, hDCScreen);
    lpSizeInHiMetric->cx = MAP_PIX_TO_LOGHIM(lpSizeInPix->cx, nPixelsPerInchX);
    lpSizeInHiMetric->cy = MAP_PIX_TO_LOGHIM(lpSizeInPix->cy, nPixelsPerInchY);
}

void ActiveX::SetVisible(bool bVisible)
{
    Control::SetVisible(bVisible);
	if (m_hwndHost != 0 && !m_pControl->m_bWindowless) {
		fn_ShowWindow(m_hwndHost, IsVisible() ? SW_SHOW : SW_HIDE);
	}
}

void ActiveX::SetInternVisible(bool bVisible)
{
    Control::SetInternVisible(bVisible);
	if (m_hwndHost != 0 && !m_pControl->m_bWindowless) {
		fn_ShowWindow(m_hwndHost, IsVisible() ? SW_SHOW : SW_HIDE);
	}
}

void ActiveX::SetPos(RECT rc)
{
    Control::SetPos(rc);

	if (!m_bCreated) {
		DoCreateControl();
	}

	if (m_pUnk == 0) {
		return;
	}
	if (m_pControl == 0) {
		return;
	}

    SIZEL hmSize = { 0 };
    SIZEL pxSize = { 0 };
    pxSize.cx = _rcItem.right - _rcItem.left;
    pxSize.cy = _rcItem.bottom - _rcItem.top;
    PixelToHiMetric(&pxSize, &hmSize);

    if (m_pUnk != 0) {
        m_pUnk->SetExtent(DVASPECT_CONTENT, &hmSize);
    }
    if (m_pControl->m_pInPlaceObject != 0) {
        Rect rcItem = _rcItem;
		if (!m_pControl->m_bWindowless) {
			rcItem.resetOffset();
		}
        m_pControl->m_pInPlaceObject->SetObjectRects(&rcItem, &rcItem);
    }
    if (!m_pControl->m_bWindowless) {
        zgui_assert(m_pControl->m_pWindow);
        fn_MoveWindow(*m_pControl->m_pWindow, _rcItem.left, _rcItem.top, _rcItem.right - _rcItem.left, _rcItem.bottom - _rcItem.top, TRUE);
    }
}

void ActiveX::DoPaint(HDC hDC, const RECT& rcPaint)
{
	if (!fn_IntersectRect(&_rcPaint, &rcPaint, &_rcItem)) {
		return;
	}

	if (m_pControl != 0 && m_pControl->m_bWindowless && m_pControl->m_pViewObject != 0) {
        m_pControl->m_pViewObject->Draw(DVASPECT_CONTENT, -1, 0, 0, 0, hDC, (RECTL*) &_rcItem, (RECTL*) &_rcItem, 0, 0); 
    }
}

void ActiveX::setAttribute(const String& pstrName, const String& pstrValue)
{
	if (pstrName == "clsid") {
		CreateControl(pstrValue);
	}
	else if (pstrName == "modulename") {
		SetModuleName(pstrValue);
	}
	else if (pstrName == "delaycreate") {
		SetDelayCreate(pstrValue == "true");
	}
	else {
		Control::setAttribute(pstrName, pstrValue);
	}
}

LRESULT ActiveX::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
    if( m_pControl == 0 ) return 0;
    zgui_assert(m_pControl->m_bWindowless);
    if( !m_pControl->m_bInPlaceActive ) return 0;
    if( m_pControl->m_pInPlaceObject == 0 ) return 0;
    if( !IsMouseEnabled() && uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST ) return 0;
    bool bWasHandled = true;
    if( (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) || uMsg == WM_SETCURSOR ) {
        // Mouse message only go when captured or inside rect
        DWORD dwHitResult = m_pControl->m_bCaptured ? HITRESULT_HIT : HITRESULT_OUTSIDE;
        if( dwHitResult == HITRESULT_OUTSIDE && m_pControl->m_pViewObject != 0 ) {
            IViewObjectEx* pViewEx = 0;
            m_pControl->m_pViewObject->QueryInterface(IID_IViewObjectEx, (LPVOID*) &pViewEx);
            if( pViewEx != 0 ) {
                POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                pViewEx->QueryHitPoint(DVASPECT_CONTENT, &_rcItem, ptMouse, 0, &dwHitResult);
                pViewEx->Release();
            }
        }
        if( dwHitResult != HITRESULT_HIT ) return 0;
        if( uMsg == WM_SETCURSOR ) bWasHandled = false;
    }
    else if( uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST ) {
        // Keyboard messages just go when we have focus
        if( !IsFocused() ) return 0;
    }
    else {
        switch( uMsg ) {
        case WM_HELP:
        case WM_CONTEXTMENU:
            bWasHandled = false;
            break;
        default:
            return 0;
        }
    }
    LRESULT lResult = 0;
    HRESULT Hr = m_pControl->m_pInPlaceObject->OnWindowMessage(uMsg, wParam, lParam, &lResult);
    if( Hr == S_OK ) bHandled = bWasHandled;
    return lResult;
}

bool ActiveX::IsDelayCreate() const
{
    return m_bDelayCreate;
}

void ActiveX::SetDelayCreate(bool bDelayCreate)
{
	if (m_bDelayCreate == bDelayCreate) {
		return;
	}
    if (bDelayCreate == false) {
		if (m_bCreated == false && fn_RtlCompareMemory(&m_clsid, &IID_NULL, sizeof(m_clsid)) != sizeof(m_clsid)) {
			DoCreateControl();
		}
    }
    m_bDelayCreate = bDelayCreate;
}

bool ActiveX::CreateControl(const String& pstrCLSID)
{
    CLSID clsid;
    OLECHAR szCLSID[100];

	__stosb((uint8_t*)&clsid, 0, sizeof(clsid));
	__stosb((uint8_t*)szCLSID, 0, sizeof(szCLSID));
	pstrCLSID.copyToUTF16((wchar_t*)szCLSID, lengthof(szCLSID) - 1);
	if (pstrCLSID[0] == L'{') {
		fn_CLSIDFromString(szCLSID, &clsid);
	}
	else {
		fn_CLSIDFromProgID(szCLSID, &clsid);
	}
    return CreateControl(clsid);
}

bool ActiveX::CreateControl(const CLSID clsid)
{
	if (fn_RtlCompareMemory(&clsid, &IID_NULL, sizeof(clsid)) == sizeof(clsid)) {
		return false;
	}
    m_bCreated = false;
    m_clsid = clsid;
	if (!m_bDelayCreate) {
		DoCreateControl();
	}
    return true;
}

void ActiveX::ReleaseControl()
{
    m_hwndHost = 0;
    if (m_pUnk != 0) {
        IObjectWithSite* pSite = 0;
        m_pUnk->QueryInterface(IID_IObjectWithSite, (LPVOID*) &pSite);
        if (pSite != 0) {
            pSite->SetSite(0);
            pSite->Release();
        }
        m_pUnk->Close(OLECLOSE_NOSAVE);
        m_pUnk->SetClientSite(0);
        m_pUnk->Release(); 
        m_pUnk = 0;
    }
    if (m_pControl != 0) {
        m_pControl->m_pOwner = 0;
        m_pControl->Release();
        m_pControl = 0;
    }
    _pManager->RemoveMessageFilter(this);
}

typedef HRESULT (__stdcall *DllGetClassObjectFunc)(REFCLSID rclsid, REFIID riid, LPVOID* ppv); 

bool ActiveX::DoCreateControl()
{
	ReleaseControl();
	// At this point we'll create the ActiveX control
	m_bCreated = true;
	IOleControl* pOleControl = 0;

	HRESULT Hr = -1;
	if (!m_sModuleName.isEmpty()) {
		HMODULE hModule = fn_LoadLibraryW(m_sModuleName.toWideCharPointer());
		if (hModule != 0) {
			IClassFactory* aClassFactory = 0;
			DllGetClassObjectFunc aDllGetClassObjectFunc = (DllGetClassObjectFunc)fn_GetProcAddress(hModule, "DllGetClassObject");
			Hr = aDllGetClassObjectFunc(m_clsid, IID_IClassFactory, (LPVOID*)&aClassFactory);
			if (SUCCEEDED(Hr)) {
				Hr = aClassFactory->CreateInstance(0, IID_IOleObject, (LPVOID*)&pOleControl);
			}
			aClassFactory->Release();
		}
	}
	if (FAILED(Hr)) {
		Hr = fn_CoCreateInstance(m_clsid, 0, CLSCTX_ALL, IID_IOleControl, (LPVOID*)&pOleControl);
	}
	if (FAILED(Hr)) {
		return false;
	}
	pOleControl->QueryInterface(IID_IOleObject, (LPVOID*)&m_pUnk);
	pOleControl->Release();
	if (m_pUnk == 0) {
		return false;
	}
	// Create the host too
	m_pControl = new ActiveXCtrl();
	m_pControl->m_pOwner = this;
	//m_pControl->CreateActiveXWnd();
	// More control creation stuff
	DWORD dwMiscStatus = 0;
	m_pUnk->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);
	IOleClientSite* pOleClientSite = 0;
	m_pControl->QueryInterface(IID_IOleClientSite, (LPVOID*)&pOleClientSite);
	CSafeRelease<IOleClientSite> RefOleClientSite = pOleClientSite;
	// Initialize control
	if ((dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST) != 0) {
		Hr = m_pUnk->SetClientSite(pOleClientSite);
	}
	IPersistStreamInit* pPersistStreamInit = 0;
	m_pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreamInit);
	if (pPersistStreamInit != 0) {
		Hr = pPersistStreamInit->InitNew();
		pPersistStreamInit->Release();
	}
	if (FAILED(Hr)) {
		return false;
	}
	if ((dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST) == 0) {
		m_pUnk->SetClientSite(pOleClientSite);
	}
	// Grab the view...
	Hr = m_pUnk->QueryInterface(IID_IViewObjectEx, (LPVOID*)&m_pControl->m_pViewObject);
	if (FAILED(Hr)) {
		Hr = m_pUnk->QueryInterface(IID_IViewObject2, (LPVOID*)&m_pControl->m_pViewObject);
	}
	if (FAILED(Hr)) {
		Hr = m_pUnk->QueryInterface(IID_IViewObject, (LPVOID*)&m_pControl->m_pViewObject);
	}
	// Activate and done...
	m_pUnk->SetHostNames(OLESTR("UIActiveX"), 0);
	if (_pManager != 0) {
		_pManager->SendNotify((Control*)this, ZGUI_MSGTYPE_SHOWACTIVEX, 0, 0, false);
	}
	if ((dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME) == 0) {
		Hr = m_pUnk->DoVerb(OLEIVERB_INPLACEACTIVATE, 0, pOleClientSite, 0, _pManager->GetPaintWindow(), &_rcItem);
		//::RedrawWindow(m_pManager->GetPaintWindow(), &m_rcItem, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
	}
	IObjectWithSite* pSite = 0;
	m_pUnk->QueryInterface(IID_IObjectWithSite, (LPVOID*)&pSite);
	if (pSite != 0) {
		pSite->SetSite(static_cast<IOleClientSite*>(m_pControl));
		pSite->Release();
	}
	return SUCCEEDED(Hr);
}

HRESULT ActiveX::GetControl(const IID iid, LPVOID* ppRet)
{
    zgui_assert(ppRet!=0);
    zgui_assert(*ppRet==0);
	if (ppRet == 0) {
		return E_POINTER;
	}
	if (m_pUnk == 0) {
		return E_PENDING;
	}
    return m_pUnk->QueryInterface(iid, (LPVOID*) ppRet);
}

CLSID ActiveX::GetClisd() const
{
	return m_clsid;
}

const String& ActiveX::GetModuleName() const
{
    return m_sModuleName;
}

void ActiveX::SetModuleName(const String& pstrText)
{
    m_sModuleName = pstrText;
}

} // namespace DuiLib

#endif // ZGUI_USE_ACTIVEX