#include "zgui.h"

#ifdef ZGUI_USE_ACTIVEX

#ifdef ZGUI_USE_WEBBROWSER

#include "zguiDownloadMgr.h"

#include <mshtml.h>

namespace zgui {

const String WebBrowser::CLASS_NAME = "WebBrowser";

WebBrowser::WebBrowser() :
m_pWebBrowser2(0),
_pHtmlWnd2(0),
m_pWebBrowserEventHandler(0),
m_bAutoNavi(false),
m_dwRef(0),
m_dwCookie(0)
{
	m_clsid = CLSID_WebBrowser;
}

bool WebBrowser::DoCreateControl()
{
	if (!ActiveX::DoCreateControl()) {
		return false;
	}
	_pManager->AddTranslateAccelerator(this);
	GetControl(IID_IWebBrowser2, (LPVOID*)&m_pWebBrowser2);
	if (m_bAutoNavi && !m_sHomePage.isEmpty()) {
		this->Navigate2(m_sHomePage.toWideCharPointer());
	}
	RegisterEventHandler(TRUE);
	return true;
}

void WebBrowser::ReleaseControl()
{
	m_bCreated = false;
	getManager()->RemoveTranslateAccelerator(this);
	RegisterEventHandler(FALSE);
}

WebBrowser::~WebBrowser()
{
	ReleaseControl();
}

STDMETHODIMP WebBrowser::GetTypeInfoCount( UINT *iTInfo )
{
	*iTInfo = 0;
	return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::GetTypeInfo( UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo )
{
return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::GetIDsOfNames( REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid,DISPID *rgDispId )
{
return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::Invoke( DISPID dispIdMember, REFIID riid, LCID lcid,WORD wFlags, DISPPARAMS* pDispParams,VARIANT* pVarResult, EXCEPINFO* pExcepInfo,UINT* puArgErr )
{
	
	if (fn_RtlCompareMemory(&riid, &IID_NULL, sizeof(riid)) != sizeof(riid)) {
		return E_INVALIDARG;
	}
	zgui::String mgsg((int)dispIdMember);
	mgsg << "\n";
	fn_OutputDebugStringW(mgsg.toWideCharPointer());
	switch (dispIdMember) {
		case DISPID_BEFORENAVIGATE2:
			BeforeNavigate2(pDispParams->rgvarg[6].pdispVal, pDispParams->rgvarg[5].pvarVal, pDispParams->rgvarg[4].pvarVal, pDispParams->rgvarg[3].pvarVal, pDispParams->rgvarg[2].pvarVal, pDispParams->rgvarg[1].pvarVal, pDispParams->rgvarg[0].pboolVal);
			break;
		case DISPID_COMMANDSTATECHANGE:
			CommandStateChange(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0].boolVal);
			break;
		case DISPID_NAVIGATECOMPLETE2:
			NavigateComplete2(pDispParams->rgvarg[1].pdispVal, pDispParams->rgvarg[0].pvarVal);
			break;
		case DISPID_NAVIGATEERROR:
			NavigateError(pDispParams->rgvarg[4].pdispVal, pDispParams->rgvarg[3].pvarVal, pDispParams->rgvarg[2].pvarVal, pDispParams->rgvarg[1].pvarVal, pDispParams->rgvarg[0].pboolVal);
			break;
		case DISPID_STATUSTEXTCHANGE:
			break;
			//  	case DISPID_NEWWINDOW2:
			//  		break;
		case DISPID_DOCUMENTCOMPLETE:
			DocumentComplete(pDispParams->rgvarg[1].pdispVal, pDispParams->rgvarg[0].pvarVal);
			break;
		case DISPID_NEWWINDOW3:
			NewWindow3(pDispParams->rgvarg[4].ppdispVal, pDispParams->rgvarg[3].pboolVal, pDispParams->rgvarg[2].uintVal, pDispParams->rgvarg[1].bstrVal, pDispParams->rgvarg[0].bstrVal);
			break;
			// 	case DISPID_PROPERTYCHANGE:
			// 		if (pDispParams->cArgs>0 && pDispParams->rgvarg[0].vt == VT_BSTR) {
			// 			TRACE(_T("PropertyChange(%s)\n"), pDispParams->rgvarg[0].bstrVal);
			// 		}
			// 		break;
		case DISPID_WINDOWCLOSING:
			WindowClosing(pDispParams->rgvarg[1].boolVal, pDispParams->rgvarg[0].pboolVal);
			break;
		default:
			return DISP_E_MEMBERNOTFOUND;
	}
	return S_OK;
}

STDMETHODIMP WebBrowser::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
	*ppvObject = 0;

	if (fn_RtlCompareMemory(&riid, &IID_IDocHostUIHandler, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IDocHostUIHandler*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IDispatch, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IDispatch*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IServiceProvider, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IServiceProvider*>(this);
	}
	else if (fn_RtlCompareMemory(&riid, &IID_IOleCommandTarget, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = static_cast<IOleCommandTarget*>(this);
	}

	if (*ppvObject != 0) {
		AddRef();
	}
	return *ppvObject == 0 ? E_NOINTERFACE : S_OK;
}

STDMETHODIMP_(ULONG) WebBrowser::AddRef()
{
	_InterlockedIncrement(&m_dwRef); 
	return m_dwRef;
}

STDMETHODIMP_(ULONG) WebBrowser::Release()
{
	ULONG ulRefCount = _InterlockedDecrement(&m_dwRef);
	return ulRefCount; 
}

void WebBrowser::Navigate2(const String& lpszUrl)
{
	if (lpszUrl.isEmpty()) {
		return;
	}

	if (m_pWebBrowser2) {
		Variant url;
		url.vt = VT_BSTR;
		url.bstrVal = fn_SysAllocString(lpszUrl.toWideCharPointer());
		HRESULT hr = m_pWebBrowser2->Navigate2(&url, 0, 0, 0, 0);
	}
}

void WebBrowser::Refresh()
{
	if (m_pWebBrowser2) {
		m_pWebBrowser2->Refresh();
	}
}

void WebBrowser::GoBack()
{
	if (m_pWebBrowser2)
	{
		m_pWebBrowser2->GoBack();
	}
}
void WebBrowser::GoForward()
{
	if (m_pWebBrowser2)
	{
		m_pWebBrowser2->GoForward();
	}
}

/// DWebBrowserEvents2
void WebBrowser::BeforeNavigate2( IDispatch *pDisp,VARIANT *&url,VARIANT *&Flags,VARIANT *&TargetFrameName,VARIANT *&PostData,VARIANT *&Headers,VARIANT_BOOL *&Cancel )
{
	if (m_pWebBrowserEventHandler) {
		m_pWebBrowserEventHandler->BeforeNavigate2(pDisp,url,Flags,TargetFrameName,PostData,Headers,Cancel);
	}
}

void WebBrowser::NavigateError( IDispatch *pDisp,VARIANT * &url,VARIANT *&TargetFrameName,VARIANT *&StatusCode,VARIANT_BOOL *&Cancel )
{
	if (m_pWebBrowserEventHandler) {
		m_pWebBrowserEventHandler->NavigateError(pDisp,url,TargetFrameName,StatusCode,Cancel);
	}
}

void WebBrowser::NavigateComplete2(IDispatch* pDisp, VARIANT*& url)
{
	HRESULT hr;
	ComSmartPtr<IDispatch> spDoc;
	m_pWebBrowser2->get_Document(spDoc.address());

	if (spDoc) {
		ComSmartPtr<ICustomDoc> spCustomDoc;
		hr = spDoc.QueryInterface(IID_ICustomDoc, spCustomDoc);
		if (SUCCEEDED(hr)) {
			spCustomDoc->SetUIHandler(this);
		}
	}

	if (m_pWebBrowserEventHandler) {
		m_pWebBrowserEventHandler->NavigateComplete2(pDisp, url);
	}
}

void WebBrowser::DocumentComplete(IDispatch *pDisp, VARIANT *url)
{
	if (m_pWebBrowserEventHandler) {
		m_pWebBrowserEventHandler->DocumentComplete(pDisp, url);
	}
}

void WebBrowser::WindowClosing(VARIANT_BOOL IsChildWindow, VARIANT_BOOL *Cancel)
{
	if (m_pWebBrowserEventHandler) {
		m_pWebBrowserEventHandler->WindowClosing(IsChildWindow, Cancel);
	}
}

void WebBrowser::ProgressChange( LONG nProgress, LONG nProgressMax )
{
	if (m_pWebBrowserEventHandler) {
		m_pWebBrowserEventHandler->ProgressChange(nProgress,nProgressMax);
	}
}

void WebBrowser::NewWindow3( IDispatch **pDisp, VARIANT_BOOL *&Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl )
{
	if (m_pWebBrowserEventHandler) {
		m_pWebBrowserEventHandler->NewWindow3(pDisp,Cancel,dwFlags,bstrUrlContext,bstrUrl);
	}
}
void WebBrowser::CommandStateChange(long Command,VARIANT_BOOL Enable)
{
	if (m_pWebBrowserEventHandler) {
		m_pWebBrowserEventHandler->CommandStateChange(Command,Enable);
	}
}

// IDownloadManager
STDMETHODIMP WebBrowser::Download( /* [in] */ IMoniker *pmk, /* [in] */ IBindCtx *pbc, /* [in] */ DWORD dwBindVerb, /* [in] */ LONG grfBINDF, /* [in] */ BINDINFO *pBindInfo, /* [in] */ LPCOLESTR pszHeaders, /* [in] */ LPCOLESTR pszRedir, /* [in] */ UINT uiCP )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->Download(pmk,pbc,dwBindVerb,grfBINDF,pBindInfo,pszHeaders,pszRedir,uiCP);
	}
	return S_OK;
}

// IDocHostUIHandler
STDMETHODIMP WebBrowser::ShowContextMenu( DWORD dwID, POINT* pptPosition, IUnknown* pCommandTarget, IDispatch* pDispatchObjectHit )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->ShowContextMenu(dwID, pptPosition, pCommandTarget, pDispatchObjectHit);
	}
	return S_FALSE;
}

STDMETHODIMP WebBrowser::GetHostInfo( DOCHOSTUIINFO* pInfo )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->GetHostInfo(pInfo);
	}
	return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::ShowUI( DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->ShowUI(dwID,pActiveObject,pCommandTarget,pFrame,pDoc);
	}
	return S_OK;
}

STDMETHODIMP WebBrowser::HideUI()
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->HideUI();
	}
	return S_OK;
}

STDMETHODIMP WebBrowser::UpdateUI()
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->UpdateUI();
	}
	return S_OK;
}

STDMETHODIMP WebBrowser::EnableModeless( BOOL fEnable )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->EnableModeless(fEnable);
	}
	return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::OnDocWindowActivate( BOOL fActivate )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->OnDocWindowActivate(fActivate);
	}
	return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::OnFrameWindowActivate( BOOL fActivate )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->OnFrameWindowActivate(fActivate);
	}
	return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::ResizeBorder( LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fFrameWindow )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->ResizeBorder(prcBorder, pUIWindow, fFrameWindow);
	}
	return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::TranslateAccelerator( LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID);
	}
	return S_FALSE;
}

LRESULT WebBrowser::TranslateAccelerator(MSG *pMsg)
{
	if (pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) {
		return S_FALSE;
	}

	if (m_pWebBrowser2 == 0) {
		return E_NOTIMPL;
	}

	BOOL bIsChild = FALSE;
	HWND hTempWnd = 0;
	HWND hWndFocus = fn_GetFocus();

	hTempWnd = hWndFocus;
	while (hTempWnd != 0) {
		if (hTempWnd == m_hwndHost) {
			bIsChild = TRUE;
			break;
		}
		hTempWnd = fn_GetParent(hTempWnd);
	}
	if (!bIsChild) {
		return S_FALSE;
	}

	IOleInPlaceActiveObject *pObj;
	if (FAILED(m_pWebBrowser2->QueryInterface(IID_IOleInPlaceActiveObject, (LPVOID *)&pObj))) {
		return S_FALSE;
	}

	HRESULT hResult = pObj->TranslateAccelerator(pMsg);
	pObj->Release();
	return hResult;
}

STDMETHODIMP WebBrowser::GetOptionKeyPath( LPOLESTR* pchKey, DWORD dwReserved )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->GetOptionKeyPath(pchKey, dwReserved);
	}
	return E_NOTIMPL;
}

STDMETHODIMP WebBrowser::GetDropTarget( IDropTarget* pDropTarget, IDropTarget** ppDropTarget )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->GetDropTarget(pDropTarget, ppDropTarget);
	}
	return S_FALSE;	// 使用系统拖拽
}

STDMETHODIMP WebBrowser::GetExternal( IDispatch** ppDispatch )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->GetExternal(ppDispatch);
	}
	return S_FALSE;
}

STDMETHODIMP WebBrowser::TranslateUrl( DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->TranslateUrl(dwTranslate, pchURLIn, ppchURLOut);
	}
	else {
		*ppchURLOut = 0;
		return E_NOTIMPL;
	}
}

STDMETHODIMP WebBrowser::FilterDataObject( IDataObject* pDO, IDataObject** ppDORet )
{
	if (m_pWebBrowserEventHandler) {
		return m_pWebBrowserEventHandler->FilterDataObject(pDO, ppDORet);
	}
	else {
		*ppDORet = 0;
		return E_NOTIMPL;
	}
}

void WebBrowser::SetWebBrowserEventHandler(WebBrowserEventHandler* pEventHandler)
{
	m_pWebBrowserEventHandler = pEventHandler;
}

void WebBrowser::Refresh2(int Level)
{
	Variant vLevel;
	vLevel.vt = VT_I4;
	vLevel.intVal = Level;
	m_pWebBrowser2->Refresh2(&vLevel);
}

void WebBrowser::setAttribute(const String& pstrName, const String& pstrValue)
{
	if (pstrName == "homepage") {
		m_sHomePage = pstrValue;
	}
	else if (pstrName == "autonavi") {
		m_bAutoNavi = (pstrValue == "true");
	}
    else {
		ActiveX::setAttribute(pstrName, pstrValue);
    }
}

void WebBrowser::NavigateHomePage()
{
	if (!m_sHomePage.isEmpty()) {
		this->NavigateUrl(m_sHomePage);
	}
}

void WebBrowser::NavigateUrl(const String& lpszUrl)
{
	if (m_pWebBrowser2 && !lpszUrl.isEmpty()) {
		m_pWebBrowser2->Navigate((BSTR)fn_SysAllocString(lpszUrl.toWideCharPointer()),0,0,0,0);
	}
}

const String& WebBrowser::getClass() const
{
	return CLASS_NAME;
}

LPVOID WebBrowser::getInterface(const String& name)
{
    if (name == "WebBrowser") {
        return static_cast<WebBrowser*>(this);
    }
	return ActiveX::getInterface(name);
}

void WebBrowser::SetHomePage(const String& lpszUrl)
{
	m_sHomePage = lpszUrl;
}

const String& WebBrowser::GetHomePage()
{
	return m_sHomePage;
}

void WebBrowser::SetAutoNavigation(bool bAuto)
{
	if (m_bAutoNavi == bAuto) {
		return;
	}

	m_bAutoNavi = bAuto;
}

bool WebBrowser::IsAutoNavigation()
{
	return m_bAutoNavi;
}

STDMETHODIMP WebBrowser::QueryService(REFGUID guidService, REFIID riid, void** ppvObject)
{
	HRESULT hr = E_NOINTERFACE;
	*ppvObject = 0;
	
	if (fn_RtlCompareMemory(&guidService, &SID_SDownloadManager, sizeof(guidService)) == sizeof(guidService) && fn_RtlCompareMemory(&riid, &IID_IDownloadManager, sizeof(riid)) == sizeof(riid)) {
		*ppvObject = this;
		return S_OK;
	}

	return hr;
}

HRESULT WebBrowser::RegisterEventHandler(BOOL inAdvise)
{
	ComSmartPtr<IWebBrowser2> pWebBrowser;
	ComSmartPtr<IConnectionPointContainer>  pCPC;
	ComSmartPtr<IConnectionPoint> pCP;
	HRESULT hr = GetControl(IID_IWebBrowser2, (void**)pWebBrowser.address());
	if (FAILED(hr)) {
		return hr;
	}
	hr = pWebBrowser->QueryInterface(IID_IConnectionPointContainer, (void**)pCPC.address());
	if (FAILED(hr)) {
		return hr;
	}
	hr = pCPC->FindConnectionPoint(DIID_DWebBrowserEvents2, pCP.address());
	if (FAILED(hr)) {
		return hr;
	}

	if (inAdvise) {
		hr = pCP->Advise((IDispatch*)this, &m_dwCookie);
	}
	else {
		hr = pCP->Unadvise(m_dwCookie);
	}
	return hr;
}

DISPID WebBrowser::FindId(IDispatch *pObj, LPOLESTR pName)
{
	DISPID id = 0;
	if (FAILED(pObj->GetIDsOfNames(IID_NULL, &pName, 1, LOCALE_SYSTEM_DEFAULT, &id))) {
		id = -1;
	}
	return id;
}

HRESULT WebBrowser::InvokeMethod(IDispatch *pObj, LPOLESTR pMehtod, VARIANT *pVarResult, VARIANT *ps, int cArgs)
{
	DISPID dispid = FindId(pObj, pMehtod);
	if (dispid == -1) {
		return E_FAIL;
	}

	DISPPARAMS dispparams;
	dispparams.cArgs = cArgs;
	dispparams.rgvarg = ps;
	dispparams.cNamedArgs = 0;
	dispparams.rgdispidNamedArgs = 0;

	return pObj->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparams, pVarResult, 0, 0);
}

HRESULT WebBrowser::GetProperty(IDispatch *pObj, LPOLESTR pName, VARIANT *pValue)
{
	DISPID dispid = FindId(pObj, pName);
	if (dispid == -1) {
		return E_FAIL;
	}

	DISPPARAMS ps;
	ps.cArgs = 0;
	ps.rgvarg = 0;
	ps.cNamedArgs = 0;
	ps.rgdispidNamedArgs = 0;

	return pObj->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &ps, pValue, 0, 0);
}

HRESULT WebBrowser::SetProperty(IDispatch *pObj, LPOLESTR pName, VARIANT *pValue)
{
	DISPID dispid = FindId(pObj, pName);
	if (dispid == -1) {
		return E_FAIL;
	}

	DISPPARAMS ps;
	ps.cArgs = 1;
	ps.rgvarg = pValue;
	ps.cNamedArgs = 0;
	ps.rgdispidNamedArgs = 0;

	return pObj->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT, &ps, 0, 0, 0);
}

IDispatch* WebBrowser::GetHtmlWindow()
{
	IDispatch* pDp = 0;
	HRESULT hr;
	if (m_pWebBrowser2) {
		hr = m_pWebBrowser2->get_Document(&pDp);
	}

	if (FAILED(hr)) {
		return 0;
	}

	ComSmartPtr<IHTMLDocument2> pHtmlDoc2 = pDp;

	if ((IHTMLDocument2*)pHtmlDoc2 == 0) {
		return 0;
	}

	hr = pHtmlDoc2->get_parentWindow(&_pHtmlWnd2);

	if (FAILED(hr)) {
		return 0;
	}

	IDispatch *pHtmlWindown = 0;
	hr = _pHtmlWnd2->QueryInterface(IID_IDispatch, (void**)&pHtmlWindown);
	if (FAILED(hr)) {
		return 0;
	}

	return pHtmlWindown;
}

IWebBrowser2* WebBrowser::GetWebBrowser2(void)
{
	return m_pWebBrowser2;
}

HRESULT STDMETHODCALLTYPE WebBrowser::QueryStatus(__RPC__in_opt const GUID *pguidCmdGroup, ULONG cCmds, __RPC__inout_ecount_full(cCmds) OLECMD prgCmds[], __RPC__inout_opt OLECMDTEXT *pCmdText)
{
	HRESULT hr = OLECMDERR_E_NOTSUPPORTED;
	return hr;
}

HRESULT STDMETHODCALLTYPE WebBrowser::Exec(__RPC__in_opt const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, __RPC__in_opt VARIANT *pvaIn, __RPC__inout_opt VARIANT *pvaOut)
{
	HRESULT hr = S_OK;
	
	if (pguidCmdGroup && fn_RtlCompareMemory(pguidCmdGroup, &CGID_DocHostCommandHandler, sizeof(CGID_DocHostCommandHandler)) == sizeof(CGID_DocHostCommandHandler)) {
		switch (nCmdID) {
			case OLECMDID_SHOWSCRIPTERROR: {
				IHTMLDocument2* pDoc = 0;
				IHTMLWindow2* pWindow = 0;
				IHTMLEventObj* pEventObj = 0;
				BSTR rgwszNames[5] = {
					fn_SysAllocString(L"errorLine"),
					fn_SysAllocString(L"errorCharacter"),
					fn_SysAllocString(L"errorCode"),
					fn_SysAllocString(L"errorMessage"),
					fn_SysAllocString(L"errorUrl")
				};
				DISPID rgDispIDs[5];
				VARIANT rgvaEventInfo[5];
				DISPPARAMS params;
				BOOL fContinueRunningScripts = true;
				int i;

				params.cArgs = 0;
				params.cNamedArgs = 0;

				// Get the document that is currently being viewed.
				hr = pvaIn->punkVal->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc);
				// Get document.parentWindow.
				hr = pDoc->get_parentWindow(&pWindow);
				pDoc->Release();
				// Get the window.event object.
				hr = pWindow->get_event(&pEventObj);
				// Get the error info from the window.event object.
				for (i = 0; i < 5; ++i) {
					// Get the property's dispID.
					hr = pEventObj->GetIDsOfNames(IID_NULL, &rgwszNames[i], 1, LOCALE_SYSTEM_DEFAULT, &rgDispIDs[i]);
					// Get the value of the property.
					hr = pEventObj->Invoke(rgDispIDs[i], IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &params, &rgvaEventInfo[i], 0, 0);
					fn_SysFreeString(rgwszNames[i]);
				}

				// At this point, you would normally alert the user with 
				// the information about the error, which is now contained
				// in rgvaEventInfo[]. Or, you could just exit silently.

				(*pvaOut).vt = VT_BOOL;
				if (fContinueRunningScripts) {
					// Continue running scripts on the page.
					(*pvaOut).boolVal = VARIANT_TRUE;
				}
				else {
					// Stop running scripts on the page.
					(*pvaOut).boolVal = VARIANT_FALSE;
				}
				break;
			}
			default:
				hr = OLECMDERR_E_NOTSUPPORTED;
				break;
		}
	}
	else {
		hr = OLECMDERR_E_UNKNOWNGROUP;
	}
	return hr;
}

}

#endif // ZGUI_USE_WEBBROWSER

#endif // ZGUI_USE_ACTIVEX
