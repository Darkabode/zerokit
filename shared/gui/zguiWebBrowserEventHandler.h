#ifndef _ZGUI_WEBBROWSEREVENTHANDLER_H_
#define _ZGUI_WEBBROWSEREVENTHANDLER_H_

#include <ExDisp.h>
#include <ExDispid.h>

namespace zgui {

class WebBrowserEventHandler
{
public:
	WebBrowserEventHandler() {}
	~WebBrowserEventHandler() {}

	virtual void BeforeNavigate2( IDispatch *pDisp,VARIANT *&url,VARIANT *&Flags,VARIANT *&TargetFrameName,VARIANT *&PostData,VARIANT *&Headers,VARIANT_BOOL *&Cancel ) {}
	virtual void NavigateError(IDispatch *pDisp,VARIANT * &url,VARIANT *&TargetFrameName,VARIANT *&StatusCode,VARIANT_BOOL *&Cancel) {}
	virtual void NavigateComplete2(IDispatch *pDisp,VARIANT *&url){}
	virtual void ProgressChange(LONG nProgress, LONG nProgressMax){}
	virtual void NewWindow3(IDispatch **pDisp, VARIANT_BOOL *&Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl){}
	virtual void CommandStateChange(long Command,VARIANT_BOOL Enable){}
	virtual void DocumentComplete(IDispatch *pDisp, VARIANT *url){}
	virtual void WindowClosing(VARIANT_BOOL IsChildWindow, VARIANT_BOOL *Cancel){}
	// interface IDocHostUIHandler
	virtual HRESULT STDMETHODCALLTYPE ShowContextMenu(
		/* [in] */ DWORD dwID,
		/* [in] */ POINT __RPC_FAR *ppt,
		/* [in] */ IUnknown __RPC_FAR *pcmdtReserved,
		/* [in] */ IDispatch __RPC_FAR *pdispReserved)
	{
		return S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE GetHostInfo(
		/* [out][in] */ DOCHOSTUIINFO __RPC_FAR *pInfo)
	{
		// 		if (pInfo != NULL)
		// 		{
		// 			pInfo->dwFlags |= DOCHOSTUIFLAG_NO3DBORDER;
		// 		}
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE ShowUI(
		/* [in] */ DWORD dwID,
		/* [in] */ IOleInPlaceActiveObject __RPC_FAR *pActiveObject,
		/* [in] */ IOleCommandTarget __RPC_FAR *pCommandTarget,
		/* [in] */ IOleInPlaceFrame __RPC_FAR *pFrame,
		/* [in] */ IOleInPlaceUIWindow __RPC_FAR *pDoc)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE HideUI( void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE UpdateUI( void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE EnableModeless(
		/* [in] */ BOOL fEnable)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(
		/* [in] */ BOOL fActivate)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(
		/* [in] */ BOOL fActivate)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE ResizeBorder(
		/* [in] */ LPCRECT prcBorder,
		/* [in] */ IOleInPlaceUIWindow __RPC_FAR *pUIWindow,
		/* [in] */ BOOL fRameWindow)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(
		/* [in] */ LPMSG lpMsg,
		/* [in] */ const GUID __RPC_FAR *pguidCmdGroup,
		/* [in] */ DWORD nCmdID)
	{
		return S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath(
		/* [out] */ LPOLESTR __RPC_FAR *pchKey,
		/* [in] */ DWORD dw)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDropTarget(
		/* [in] */ IDropTarget __RPC_FAR *pDropTarget,
		/* [out] */ IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget)
	{
		return S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE GetExternal(
		/* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch)
	{
		return S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE TranslateUrl(
		/* [in] */ DWORD dwTranslate,
		/* [in] */ OLECHAR __RPC_FAR *pchURLIn,
		/* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut)
	{
		*ppchURLOut = 0;
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE FilterDataObject(
		/* [in] */ IDataObject __RPC_FAR *pDO,
		/* [out] */ IDataObject __RPC_FAR *__RPC_FAR *ppDORet)
	{
		*ppDORet = 0;
		return E_NOTIMPL;
	}

	// 	virtual HRESULT STDMETHODCALLTYPE GetOverrideKeyPath( 
	// 		/* [annotation][out] */ 
	// 		__deref_out  LPOLESTR *pchKey,
	// 		/* [in] */ DWORD dw)
	// 	{
	// 		return E_NOTIMPL;
	// 	}

	// IDownloadManager
	virtual HRESULT STDMETHODCALLTYPE Download( 
		/* [in] */ IMoniker *pmk,
		/* [in] */ IBindCtx *pbc,
		/* [in] */ DWORD dwBindVerb,
		/* [in] */ LONG grfBINDF,
		/* [in] */ BINDINFO *pBindInfo,
		/* [in] */ LPCOLESTR pszHeaders,
		/* [in] */ LPCOLESTR pszRedir,
		/* [in] */ UINT uiCP)
	{
		return S_OK;
	}
};

}

#endif // _ZGUI_WEBBROWSEREVENTHANDLER_H_
