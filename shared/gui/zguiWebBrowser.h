#ifndef __ZGUI_WEBBROWSER_H_
#define __ZGUI_WEBBROWSER_H_

#ifdef ZGUI_USE_ACTIVEX

#ifdef ZGUI_USE_WEBBROWSER

#include "zguiWebBrowserEventHandler.h"
#include <ExDisp.h>

namespace zgui {

class WebBrowser : public ActiveX, public IDocHostUIHandler, public IServiceProvider, public IOleCommandTarget, public IDispatch, public ITranslateAccelerator
{
public:
	WebBrowser();
	virtual ~WebBrowser();

	virtual const String& getClass() const;
	virtual LPVOID getInterface(const String& pstrName);


	void SetHomePage(const String& lpszUrl);
	const String& GetHomePage();

	void SetAutoNavigation(bool bAuto = true);
	bool IsAutoNavigation();

	void SetWebBrowserEventHandler(WebBrowserEventHandler* pEventHandler);
	void Navigate2(const String& lpszUrl);
	void Refresh();
	void Refresh2(int Level);
	void GoBack();
	void GoForward();
	void NavigateHomePage();
	void NavigateUrl(const String& lpszUrl);
	virtual bool DoCreateControl();
	IWebBrowser2* GetWebBrowser2(void);
	IDispatch* GetHtmlWindow();
	static DISPID FindId(IDispatch *pObj, LPOLESTR pName);
	static HRESULT InvokeMethod(IDispatch *pObj, LPOLESTR pMehtod, VARIANT *pVarResult, VARIANT *ps, int cArgs);
	static HRESULT GetProperty(IDispatch *pObj, LPOLESTR pName, VARIANT *pValue);
	static HRESULT SetProperty(IDispatch *pObj, LPOLESTR pName, VARIANT *pValue);

protected:
	IWebBrowser2* m_pWebBrowser2; //дЇААЖчЦёХл
	IHTMLWindow2* _pHtmlWnd2;
	LONG m_dwRef;
	DWORD m_dwCookie;

	virtual void ReleaseControl();
	HRESULT RegisterEventHandler(BOOL inAdvise);
	virtual void setAttribute(const String& pstrName, const String& pstrValue);
	String m_sHomePage;	// Д¬ИПТіГж
	bool m_bAutoNavi;	// КЗ·сЖф¶ЇК±ґтїЄД¬ИПТіГж
	WebBrowserEventHandler* m_pWebBrowserEventHandler;	//дЇААЖчКВјюґ¦Ан

	// DWebBrowserEvents2
	void BeforeNavigate2( IDispatch *pDisp,VARIANT *&url,VARIANT *&Flags,VARIANT *&TargetFrameName,VARIANT *&PostData,VARIANT *&Headers,VARIANT_BOOL *&Cancel );
	void NavigateError(IDispatch *pDisp,VARIANT * &url,VARIANT *&TargetFrameName,VARIANT *&StatusCode,VARIANT_BOOL *&Cancel);
	void NavigateComplete2(IDispatch *pDisp,VARIANT *&url);
	void ProgressChange(LONG nProgress, LONG nProgressMax);
	void NewWindow3(IDispatch **pDisp, VARIANT_BOOL *&Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl);
	void CommandStateChange(long Command,VARIANT_BOOL Enable);
	void DocumentComplete(IDispatch *pDisp, VARIANT *url);
	void WindowClosing(VARIANT_BOOL IsChildWindow, VARIANT_BOOL *Cancel);

public:

	// IUnknown
	STDMETHOD_(ULONG,AddRef)();
	STDMETHOD_(ULONG,Release)();
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObject);

	// IDispatch
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( __RPC__out UINT *pctinfo );
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( UINT iTInfo, LCID lcid, __RPC__deref_out_opt ITypeInfo **ppTInfo );
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( __RPC__in REFIID riid, __RPC__in_ecount_full(cNames ) LPOLESTR *rgszNames, UINT cNames, LCID lcid, __RPC__out_ecount_full(cNames) DISPID *rgDispId);
	virtual HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr );

	// IDocHostUIHandler
	STDMETHOD(ShowContextMenu)(DWORD dwID, POINT* pptPosition, IUnknown* pCommandTarget, IDispatch* pDispatchObjectHit);
	STDMETHOD(GetHostInfo)(DOCHOSTUIINFO* pInfo);
	STDMETHOD(ShowUI)(DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc);
	STDMETHOD(HideUI)();
	STDMETHOD(UpdateUI)();
	STDMETHOD(EnableModeless)(BOOL fEnable);
	STDMETHOD(OnDocWindowActivate)(BOOL fActivate);
	STDMETHOD(OnFrameWindowActivate)(BOOL fActivate);
	STDMETHOD(ResizeBorder)(LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fFrameWindow);
	STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID);
	STDMETHOD(GetOptionKeyPath)(LPOLESTR* pchKey, DWORD dwReserved);
	STDMETHOD(GetDropTarget)(IDropTarget* pDropTarget, IDropTarget** ppDropTarget);
	STDMETHOD(GetExternal)(IDispatch** ppDispatch);
	STDMETHOD(TranslateUrl)(DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut);
	STDMETHOD(FilterDataObject)(IDataObject* pDO, IDataObject** ppDORet);


	// IServiceProvider
	STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void** ppvObject);

	// IOleCommandTarget
	virtual HRESULT STDMETHODCALLTYPE QueryStatus(__RPC__in_opt const GUID *pguidCmdGroup, ULONG cCmds, __RPC__inout_ecount_full(cCmds) OLECMD prgCmds[], __RPC__inout_opt OLECMDTEXT *pCmdText);
	virtual HRESULT STDMETHODCALLTYPE Exec(__RPC__in_opt const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, __RPC__in_opt VARIANT *pvaIn, __RPC__inout_opt VARIANT *pvaOut);

	// IDownloadManager
	STDMETHOD(Download)(
		/* [in] */ IMoniker *pmk,
		/* [in] */ IBindCtx *pbc,
		/* [in] */ DWORD dwBindVerb,
		/* [in] */ LONG grfBINDF,
		/* [in] */ BINDINFO *pBindInfo,
		/* [in] */ LPCOLESTR pszHeaders,
		/* [in] */ LPCOLESTR pszRedir,
		/* [in] */ UINT uiCP);

	// ITranslateAccelerator
	virtual LRESULT TranslateAccelerator(MSG* pMsg);

private:
	static const String CLASS_NAME;
};

} // namespace DuiLib

#endif // ZGUI_USE_WEBBROWSER

#endif // ZGUI_USE_ACTIVEX

#endif // __ZGUI_WEBBROWSER_H_