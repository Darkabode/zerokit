#ifndef __UIWEBBROWSER_H__
#define __UIWEBBROWSER_H__

#ifdef ZGUI_USE_ACTIVEX

#ifdef ZGUI_USE_WEBBROWSER

#include "zguiWebBrowserEventHandler.h"

namespace zgui
{
	class CWebBrowserUI
		: public CActiveXUI
		, public IDispatch
		, public IDocHostUIHandler
		, public IDownloadManager
		, public ITranslateAccelerator
	{
	public:
		/// 构造函数
		CWebBrowserUI();
		virtual ~CWebBrowserUI();

		void SetHomePage(LPCTSTR lpszUrl);
		LPCTSTR GetHomePage();

		void SetAutoNavigation(bool bAuto = TRUE);
		bool IsAutoNavigation();

		void SetWebBrowserEventHandler(CWebBrowserEventHandler* pEventHandler);
		void Navigate2(LPCTSTR lpszUrl);
		void Refresh();
		void Refresh2(int Level);
		void NavigateHomePage();
		void NavigateUrl(LPCTSTR lpszUrl);
		virtual bool DoCreateControl();

	protected:
		IWebBrowser2* m_pWebBrowser2; //浏览器指针
		LONG m_dwRef;
		virtual void ReleaseControl();
		virtual void SetAttribute(const String& pstrName, const String& pstrValue);
		CDuiString m_sHomePage;	// 默认页面
		bool m_bAutoNavi;	// 是否启动时打开默认页面
		CWebBrowserEventHandler* m_pWebBrowserEventHandler;	//浏览器事件处理

		// DWebBrowserEvents2
		void BeforeNavigate2( IDispatch *pDisp,VARIANT *&url,VARIANT *&Flags,VARIANT *&TargetFrameName,VARIANT *&PostData,VARIANT *&Headers,VARIANT_BOOL *&Cancel );
		void NavigateError(IDispatch *pDisp,VARIANT * &url,VARIANT *&TargetFrameName,VARIANT *&StatusCode,VARIANT_BOOL *&Cancel);
		void NavigateComplete2(IDispatch *pDisp,VARIANT *&url);
		void ProgressChange(LONG nProgress, LONG nProgressMax);
		void NewWindow3(IDispatch **pDisp, VARIANT_BOOL *&Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl);
		void CommandStateChange(long Command,VARIANT_BOOL Enable);

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

		// ITranslateAccelerator
		virtual LRESULT TranslateAccelerator( MSG *pMsg );

		STDMETHOD(GetOptionKeyPath)(LPOLESTR* pchKey, DWORD dwReserved);
		STDMETHOD(GetDropTarget)(IDropTarget* pDropTarget, IDropTarget** ppDropTarget);
		STDMETHOD(GetExternal)(IDispatch** ppDispatch);
		STDMETHOD(TranslateUrl)(DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut);
		STDMETHOD(FilterDataObject)(IDataObject* pDO, IDataObject** ppDORet);

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

		virtual LPCTSTR GetClass() const;
		virtual LPVOID GetInterface( LPCTSTR pstrName );

	};
} // namespace DuiLib

#endif // ZGUI_USE_WEBBROWSER

#endif // ZGUI_USE_ACTIVEX

#endif // __UIWEBBROWSER_H__