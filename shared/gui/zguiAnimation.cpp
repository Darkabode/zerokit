#include "zgui.h"

#ifdef ZGUI_USE_ANIMATION

namespace zgui {

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

#ifndef PI
#define PI 3.1415926535897932384626433832795029L
#endif

template< class T >
class CSafeRelease
{
public:
	CSafeRelease(T* p) : m_p(p) { };
	~CSafeRelease() { if (m_p != 0) m_p->Release(); };
	T* Detach() { T* t = m_p; m_p = 0; return t; };
	T* m_p;
};

Animation::Animation()
{

}

void Animation::setParams(UITYPE_ANIM AnimType, DWORD dwStartTick, DWORD dwDuration, COLORREF clrBack, COLORREF clrKey, RECT rcFrom, int xtrans, int ytrans, int ztrans, int alpha, float zrot)
{
	this->AnimType = AnimType;
	this->dwStartTick = dwStartTick;
	this->dwDuration = dwDuration;
	data.plot.clrBack = clrBack;
	data.plot.clrKey = clrKey;
	data.plot.rcFrom = rcFrom;
	data.plot.mFrom.xtrans = xtrans;
	data.plot.mFrom.ytrans = ytrans;
	data.plot.mFrom.ztrans = ztrans;
	data.plot.mFrom.alpha = alpha;
	data.plot.mFrom.zrot = zrot;
	data.plot.iInterpolate = INTERPOLATE_COS;
}

Animation::~Animation( void )
{
}



AnimationSpooler::AnimationSpooler() :
m_hWnd(0),
m_bIsAnimating(false),
m_bIsInitialized(false),
m_pD3D(0),
m_p3DDevice(0),
m_p3DBackSurface(0),
m_nBuffers(0)
{
	__stosb((uint8_t*)m_p3DVertices, 0, sizeof(m_p3DVertices));
	__stosb((uint8_t*)m_p3DTextures, 0, sizeof(m_p3DTextures));
}

AnimationSpooler::~AnimationSpooler()
{
	cancelJobs();
}

bool AnimationSpooler::init(HWND hWnd)
{
	if (m_bIsInitialized) {
		return true;
	}
	// This is our master window
	m_hWnd = hWnd;
	// Gather window information
	RECT rcWindow = { 0 };
	fn_GetWindowRect(hWnd, &rcWindow);
	if (fn_IsRectEmpty(&rcWindow)) {
		return false;
	}
	// Is window topmost?
	HWND hWndFocus = hWnd;
	while (fn_GetParent(hWndFocus) != 0) {
		hWndFocus = fn_GetParent(hWndFocus);
	}

	// Is DirectX v9 available at all?
	if (fn_Direct3DCreate9 == 0) {
		return false;
	}

	// Initialize Direct3D
	// Fortunately we can delay-load the DirectX9 library so we
	// don't actually have a link dependency on it.
	if (m_pD3D != 0) {
		m_pD3D->Release();
	}
	m_pD3D = fn_Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pD3D == 0) {
		return false;
	}
	HRESULT Hr;
	D3DDISPLAYMODE d3ddm;
	__stosb((uint8_t*)&d3ddm, 0, sizeof(d3ddm));
	Hr = m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	if (FAILED(Hr)) {
		return false;
	}
	m_ColorFormat = d3ddm.Format;
	Hr = m_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_ColorFormat, m_ColorFormat, TRUE);
	if (FAILED(Hr)) {
		return false;
	}
	D3DPRESENT_PARAMETERS d3dpp;
	__stosb((uint8_t*)&d3dpp, 0, sizeof(d3dpp));
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;; //D3DSWAPEFFECT_FLIP
	d3dpp.Windowed = TRUE;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferCount = 1;
	d3dpp.BackBufferFormat = m_ColorFormat;
	d3dpp.EnableAutoDepthStencil = FALSE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;
	Hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndFocus, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_p3DDevice);
	if (FAILED(Hr)) {
		return false;
	}
	// Check device caps
	D3DCAPS9 caps;
	Hr = m_p3DDevice->GetDeviceCaps(&caps);
	if (caps.MaxTextureWidth < 128) {
		return false;
	}
	if ((caps.Caps3 & D3DCAPS3_COPY_TO_VIDMEM) == 0) {
		return false;
	}
	if (FAILED(Hr)) {
		return false;
	}
	// Set viewport
	D3DVIEWPORT9 vp;
	vp.X = vp.Y = 0;
	vp.Width = rcWindow.right - rcWindow.left;
	vp.Height = rcWindow.bottom - rcWindow.top;
	vp.MinZ = 0.0;
	vp.MaxZ = 1.0;
	Hr = m_p3DDevice->SetViewport(&vp);
	if (FAILED(Hr)) {
		return false;
	}
	// Set the render flags.
	m_p3DDevice->SetRenderState(D3DRS_COLORVERTEX, TRUE);
	m_p3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_p3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_p3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	// Set miscellaneous render states
	m_p3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	m_p3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_p3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_p3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	m_p3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE); 
	m_p3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	m_p3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE); 
	m_p3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);     // Use alpha from texture
	m_p3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);     // Use vertex alpha 
	m_p3DDevice->SetVertexShader(0);
	// Signal go...
	m_bIsInitialized = true;
	return true;
}

void AnimationSpooler::addJob(Animation* pJob)
{
	m_aJobs.add(pJob);
}

bool AnimationSpooler::isJobScheduled() const
{
	return m_aJobs.size() > 0 && !m_bIsAnimating;
}

bool AnimationSpooler::prepareAnimation(HWND hWnd)
{
	if (!m_bIsInitialized) {
		return false;
	}
	// Release old image of window
	if (m_p3DBackSurface != 0) {
		m_p3DBackSurface->Release();
	}
	m_p3DBackSurface= 0;
	// Create the backdrop surface
	RECT rcClient;
	fn_GetClientRect(m_hWnd, &rcClient);
	int cx = rcClient.right - rcClient.left;
	int cy = rcClient.bottom - rcClient.top;
	HRESULT Hr = m_p3DDevice->CreateOffscreenPlainSurface(cx, cy, m_ColorFormat, D3DPOOL_SYSTEMMEM, &m_p3DBackSurface, 0);
	if (FAILED(Hr)) {
		return false;
	}
	// Paint the background
	HDC hDC = 0;
	Hr = m_p3DBackSurface->GetDC(&hDC);
	if (FAILED(Hr)) {
		return false;
	}
	fn_SendMessageW(hWnd, WM_PRINTCLIENT, (WPARAM)hDC, PRF_CHECKVISIBLE | PRF_CLIENT | PRF_ERASEBKGND | PRF_CHILDREN);

	for (int i = 0; i < m_aJobs.size(); ++i) {
		Animation* pJob = m_aJobs[i];
		RECT rc = pJob->pOwner->GetPos();
//		pJob->pOwner->SetVisible(true, false);
		pJob->pOwner->DoPaint(hDC, rcClient);
	//	pJob->pOwner->SetVisible(false, true);
	}

	m_p3DBackSurface->ReleaseDC(hDC);
	// Allow each job to prepare its 3D objects
	for (int i = 0; i < m_aJobs.size(); ++i) {
		Animation* pJob = m_aJobs[i];
		switch( pJob->AnimType ) {
			case UIANIMTYPE_FLAT:
				if (!prepareJob_Flat(pJob)) {
					return false;
				}
				break;
		}
	}
	// Assign start time
	DWORD dwTick = fn_timeGetTime();
	for (int j = 0; j < m_aJobs.size(); ++j) {
		Animation* pJob = m_aJobs[j];
		pJob->dwStartTick += dwTick;
	}
	m_bIsAnimating = true;
	return true;
}

bool AnimationSpooler::render()
{
	if (!m_bIsAnimating) {
		return false;
	}
	if (!m_bIsInitialized) {
		return false;
	}
	// Get render target
	HRESULT Hr;
	LPDIRECT3DSURFACE9 p3DTargetSurface;
	Hr = m_p3DDevice->GetRenderTarget(0, &p3DTargetSurface);
	if (FAILED(Hr)) {
		return false;
	}
	CSafeRelease<IDirect3DSurface9> RefTargetSurface = p3DTargetSurface;
	// Copy backdrop
	Hr = m_p3DDevice->Clear(0, 0, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,255), 1.0f, 0L);
	Hr = m_p3DDevice->UpdateSurface(m_p3DBackSurface, 0, p3DTargetSurface, 0);
	// Here begins the rendering loop.
	Hr = m_p3DDevice->BeginScene();
	if (FAILED(Hr)) {
		return false;
	}
	int nAnimated = 0;
	DWORD dwTick = fn_timeGetTime();
	for (int i = 0; i < m_aJobs.size(); ++i) {
		Animation* pJob = m_aJobs[i];
		if (dwTick < pJob->dwStartTick) {
			continue;
		}
		DWORD dwTickNow = MIN(dwTick, pJob->dwStartTick + pJob->dwDuration);
		switch (pJob->AnimType) {
			case UIANIMTYPE_FLAT:
				renderJob_Flat(pJob, p3DTargetSurface, dwTickNow);
				break;
		}
		if (dwTick < pJob->dwStartTick + pJob->dwDuration) {
			++nAnimated;
		}
	}
	m_p3DDevice->EndScene();
	m_p3DDevice->Present(0, 0, 0, 0);
	// No more frames to animate?
	if (nAnimated == 0) {
		cancelJobs();
	}
	return true;
}

void AnimationSpooler::cancelJobs()
{
	// Get rid of the animation jobs
	int i;
	for (i = 0; i < m_aJobs.size(); ++i) {
		Animation* pJob = m_aJobs[i];
		pJob->onAnimationFinished(pJob);
	}
	m_aJobs.clear();
	// Release Direct3D references
	for (i = 0; i < m_nBuffers; i++) {
		m_p3DVertices[i]->Release();
		m_p3DTextures[i]->Release();
	}
	m_nBuffers = 0;
	if (m_p3DBackSurface != 0) {
		m_p3DBackSurface->Release();
	}
	m_p3DBackSurface = 0;
	if (m_p3DDevice != 0) {
		m_p3DDevice->Release();
	}
	m_p3DDevice = 0;
	if (m_pD3D != 0) {
		m_pD3D->Release();
	}
	m_pD3D = 0;
	// Almost done...
	m_bIsAnimating = false;
	m_bIsInitialized = false;
}


/////////////////////////////////////////////////////////////////////////////////////
//
//

static double __cdecl LinearInterpolate(double y1, double y2, double mu)
{
	return y1 * (1.0 - mu) + y2 * mu;
}

static double __cdecl CosineInterpolate(double y1, double y2, double mu)
{
	double mu2 = (1.0 - fn_cos(mu * PI)) / 2.0;
	return y1 * (1.0 - mu2) + y2 * mu2;
}


/////////////////////////////////////////////////////////////////////////////////////
//
//

COLORREF AnimationSpooler::translateColor(LPDIRECT3DSURFACE9 pSurface, COLORREF clrColor) const
{
	if (clrColor == CLR_INVALID) {
		return clrColor;
	}
	// The only way to actually determine what color a certain RGB value gets, is
	// to put a pixel on the surface and taste it.
	HDC hDC = 0;
	HRESULT Hr = pSurface->GetDC(&hDC);
	if (FAILED(Hr)) {
		return false;
	}
	COLORREF clrOld = fn_GetPixel(hDC, 0, 0);
	fn_SetPixel(hDC, 0, 0, clrColor);
	clrColor = fn_GetPixel(hDC, 0, 0);
	fn_SetPixel(hDC, 0, 0, clrOld);
	pSurface->ReleaseDC(hDC);
	return clrColor;
}

bool AnimationSpooler::setColorKey(LPDIRECT3DTEXTURE9 pTexture, LPDIRECT3DSURFACE9 pSurface, int iTexSize, COLORREF clrColorKey)
{
	if (clrColorKey == CLR_INVALID) {
		return true;
	}

	// Get colorkey's red, green, and blue components
	// and put the colorkey in the texture's native format
	DWORD r = GetRValue(clrColorKey);
	DWORD g = GetGValue(clrColorKey);
	DWORD b = GetBValue(clrColorKey);
	DWORD dwColorKey = D3DCOLOR_ARGB(255,r,g,b);

	HRESULT Hr;
	LPDIRECT3DTEXTURE9 pTex = 0;
	Hr = m_p3DDevice->CreateTexture(iTexSize, iTexSize, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pTex, 0);
	if (FAILED(Hr)) {
		return false;
	}
	CSafeRelease<IDirect3DTexture9> RefTex = pTex;
	LPDIRECT3DSURFACE9 pTexSurf = 0;
	Hr = pTex->GetSurfaceLevel(0, &pTexSurf);
	if (FAILED(Hr)) {
		return false;
	}
	CSafeRelease<IDirect3DSurface9> RefTexSurf = pTexSurf;
	Hr = m_p3DDevice->GetRenderTargetData(pSurface, pTexSurf);
	if (FAILED(Hr)) {
		return false;
	}

	// Lock the texture and scan through each pixel, replacing the colorkey pixels
	D3DLOCKED_RECT d3dlr;
	Hr = pTex->LockRect(0, &d3dlr, 0, 0);
	if (FAILED(Hr)) {
		return false;
	}
	DWORD* pBits = static_cast<DWORD*>(d3dlr.pBits);
	for (int y = 0; y < iTexSize; ++y) {
		for (int x = 0; x < iTexSize; ++x) {
			if (pBits[x] == dwColorKey) {
				pBits[x] = 0x00000000;
			}
		}
		pBits += d3dlr.Pitch / sizeof(DWORD);
	}
	pTex->UnlockRect(0);

	// Copy modified data back
	POINT pt = { 0, 0 };
	RECT rcDest = { 0, 0, iTexSize, iTexSize };
	Hr = m_p3DDevice->UpdateSurface(pTexSurf, &rcDest, pSurface, &pt);

	return false;
}

bool AnimationSpooler::prepareJob_Flat(Animation* pJob)
{
	// Determine actual colorkey
	pJob->data.plot.clrKey = translateColor(m_p3DBackSurface, pJob->data.plot.clrKey);
	// Prepare surfaces
	HRESULT Hr;
	RECT rc = pJob->data.plot.rcFrom;
	int cx = rc.right - rc.left;
	int cy = rc.bottom - rc.left;
	FLOAT z = 0.1f;
	FLOAT rhw = 1.0f / (z * 990.0f + 10.0f);
	D3DCOLOR col = 0xffffffff;
	// Determine texture size
	int iTexSize = 128;
	if (cx < 64) {
		iTexSize = 64;
	}
	if (cx < 32) {
		iTexSize = 32;
	}
	FLOAT fTexSize = (FLOAT) iTexSize;
	// Start building tiles
	pJob->iBufferStart = m_nBuffers;
	for (int x = rc.left; x < rc.right; x += iTexSize) {
		for (int y = rc.top; y < rc.bottom; y += iTexSize) {
			RECT rcTile = { x, y, MIN(rc.right, x + iTexSize), MIN(rc.bottom, y + iTexSize) };
			// Adjust texture coordinates, because last tile may only use parts
			// of the texture...
			FLOAT tcoordx = (iTexSize - (x + fTexSize - rc.right)) / fTexSize;
			FLOAT tcoordy = (iTexSize - (y + fTexSize - rc.bottom)) / fTexSize;
			if (tcoordx > 1.0f) tcoordx = 1.0f;
			if (tcoordy > 1.0f) tcoordy = 1.0f;
			// Create the vertex buffer
			CUSTOMFAN verts = {
				{ rcTile.left - 0.5f,  rcTile.top - 0.5f,    z, rhw, col, 0.0f, 0.0f },
				{ rcTile.right - 0.5f, rcTile.top - 0.5f,    z, rhw, col, tcoordx, 0.0f },
				{ rcTile.right - 0.5f, rcTile.bottom - 0.5f, z, rhw, col, tcoordx, tcoordy },
				{ rcTile.left - 0.5f,  rcTile.bottom - 0.5f, z, rhw, col, 0.0f, tcoordy }
			};
			LPDIRECT3DVERTEXBUFFER9 pVertex = 0;
			Hr = m_p3DDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &pVertex, 0);
			if (FAILED(Hr)) {
				return false;
			}
			CSafeRelease<IDirect3DVertexBuffer9> RefVertex = pVertex;
			__movsb((uint8_t*)&m_fans[m_nBuffers], (const uint8_t*)&verts, sizeof(verts));

			LPDIRECT3DTEXTURE9 pTex1 = 0;
			Hr = m_p3DDevice->CreateTexture(iTexSize, iTexSize, 1, 0, m_ColorFormat, D3DPOOL_DEFAULT, &pTex1, 0);
			if (FAILED(Hr)) {
				return false;
			}
			CSafeRelease<IDirect3DTexture9> RefTex1 = pTex1;
			LPDIRECT3DSURFACE9 pTexSurf1 = 0;
			Hr = pTex1->GetSurfaceLevel(0, &pTexSurf1);
			if (FAILED(Hr)) {
				return false;
			}
			CSafeRelease<IDirect3DSurface9> RefTexSurf1 = pTexSurf1;
			POINT pt = { 0, 0 };         
			Hr = m_p3DDevice->UpdateSurface(m_p3DBackSurface, &rcTile, pTexSurf1, &pt);
			if (FAILED(Hr)) {
				return false;
			}
			LPDIRECT3DTEXTURE9 pTex2 = 0;
			RECT rcDest = { 0, 0, rcTile.right - rcTile.left, rcTile.bottom - rcTile.top };
			Hr = m_p3DDevice->CreateTexture(iTexSize, iTexSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pTex2, 0);
			if (FAILED(Hr)) {
				return false;
			}
			CSafeRelease<IDirect3DTexture9> RefTex2 = pTex2;
			LPDIRECT3DSURFACE9 pTexSurf2 = 0;
			Hr = pTex2->GetSurfaceLevel(0, &pTexSurf2);
			if (FAILED(Hr)) {
				return false;
			}
			CSafeRelease<IDirect3DSurface9> RefTexSurf2 = pTexSurf2;
			Hr = m_p3DDevice->StretchRect(pTexSurf1, &rcDest, pTexSurf2, &rcDest, D3DTEXF_NONE);
			if (FAILED(Hr)) {
				return false;
			}
			// Replace colorkey pixels with alpha
			setColorKey(pTex2, pTexSurf2, iTexSize, pJob->data.plot.clrKey);
			// Finally, assign the texture
			m_p3DTextures[m_nBuffers] = RefTex2.Detach();
			m_p3DVertices[m_nBuffers] = RefVertex.Detach();
			m_nBuffers++;
		}
	}
	pJob->iBufferEnd = m_nBuffers;

	// Clear the background so the sprite can take its place
	COLORREF clrBack = pJob->data.plot.clrBack;
	if( clrBack != CLR_INVALID) {
		HDC hDC = 0;
		Hr = m_p3DBackSurface->GetDC(&hDC);
		if (FAILED(Hr)) {
			return false;
		}
		HBRUSH hBrush = fn_CreateSolidBrush(clrBack);
		fn_FillRect(hDC, &rc, hBrush);
		fn_DeleteObject(hBrush);
		m_p3DBackSurface->ReleaseDC(hDC);
	}
	return true;
}

bool AnimationSpooler::renderJob_Flat(const Animation* pJob, LPDIRECT3DSURFACE9 /*pSurface*/, DWORD dwTick)
{
	RECT rc = pJob->data.plot.rcFrom;
	FLOAT mu = (FLOAT)(pJob->dwStartTick + pJob->dwDuration - dwTick) / (FLOAT) pJob->dwDuration;
	FLOAT scale1 = 0.0;
	if (pJob->data.plot.iInterpolate == Animation::INTERPOLATE_LINEAR) {
		scale1 = (FLOAT)LinearInterpolate(0.0, 1.0, mu);
	}
	if (pJob->data.plot.iInterpolate == Animation::INTERPOLATE_COS) {
		scale1 = (FLOAT)CosineInterpolate(0.0, 1.0, mu);
	}
	FLOAT scale2 = 1.0f - scale1;
	D3DVECTOR ptCenter = { rc.left + ((rc.right - rc.left) / 2.0f), rc.top + ((rc.bottom - rc.top) / 2.0f) };
	FLOAT xtrans = (FLOAT) pJob->data.plot.mFrom.xtrans * scale1;
	FLOAT ytrans = (FLOAT) pJob->data.plot.mFrom.ytrans * scale1;
	FLOAT ztrans = 1.0f + ((FLOAT) fn_abs(pJob->data.plot.mFrom.ztrans) * (pJob->data.plot.mFrom.ztrans >= 0.0 ? scale1 : scale2));
	FLOAT fSin = (FLOAT) fn_sin(pJob->data.plot.mFrom.zrot * scale1);
	FLOAT fCos = (FLOAT) fn_cos(pJob->data.plot.mFrom.zrot * scale1);
	DWORD clrAlpha = ((DWORD)(0xFF - roundToInt((FLOAT)fn_abs(pJob->data.plot.mFrom.alpha) * (pJob->data.plot.mFrom.alpha >= 0 ? scale1 : scale2))) << 24) | 0xffffff;
	HRESULT Hr = 0;
	for (int iBuffer = pJob->iBufferStart; iBuffer < pJob->iBufferEnd; ++iBuffer) {
		// Lock the vertex buffer and apply transformation
		LPDIRECT3DVERTEXBUFFER9 pVBuffer = m_p3DVertices[iBuffer];
		LPVOID pVertices = 0;
		Hr = pVBuffer->Lock(0, sizeof(CUSTOMFAN), &pVertices, 0);
		if (FAILED(Hr)) {
			return false;
		}
		CUSTOMFAN verts;
		__movsb((uint8_t*)&verts, (const uint8_t*)&m_fans[iBuffer], sizeof(CUSTOMFAN));
		for (int i = 0; i < sizeof(CUSTOMFAN) / sizeof(CUSTOMVERTEX); ++i) {
			verts[i].x -= ptCenter.x;
			verts[i].y -= ptCenter.y;
			verts[i].x += xtrans;                         // Translate
			verts[i].y += ytrans;
			verts[i].x = verts[i].x * ztrans;             // Scale
			verts[i].y = verts[i].y * ztrans;
			FLOAT x = verts[i].x; FLOAT y = verts[i].y;   // Rotate around Z
			verts[i].x = x * fCos - y * fSin;
			verts[i].y = x * fSin + y * fCos;
			verts[i].x += ptCenter.x;
			verts[i].y += ptCenter.y;
			verts[i].color = clrAlpha;
		}
		__movsb((uint8_t*)pVertices, (const uint8_t*)&verts, sizeof(CUSTOMFAN));
		pVBuffer->Unlock();
		// Paint it
		Hr = m_p3DDevice->SetStreamSource(0, pVBuffer, 0, sizeof(CUSTOMVERTEX));
		Hr = m_p3DDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		Hr = m_p3DDevice->SetTexture(0, m_p3DTextures[iBuffer]);
		Hr = m_p3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
	}
	return true;
}

}

#endif // ZGUI_USE_ANIMATION
