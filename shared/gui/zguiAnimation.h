#ifndef __ZGUI_ANIMATION_H_
#define __ZGUI_ANIMATION_H_

#ifdef ZGUI_USE_ANIMATION

namespace zgui {

typedef enum
{
	UIANIMTYPE_FLAT,
	UIANIMTYPE_SWIPE,
} UITYPE_ANIM;

struct TEffectAge
{
	int _iZoom;
	uint32_t _dFillingBK;
	int _iOffectX;
	int _iOffectY;
	int _iAlpha;
	float _fRotation;
	int _iNeedTimer;
};

class Animation
{
public:
	Animation();
	~Animation(void);

	void setParams(UITYPE_ANIM AnimType, DWORD dwStartTick, DWORD dwDuration, COLORREF clrBack, COLORREF clrKey, RECT rcFrom, int xtrans, int ytrans, int ztrans, int alpha, float zrot);

	typedef enum
	{
		INTERPOLATE_LINEAR,
		INTERPOLATE_COS,
	} INTERPOLATETYPE;

	typedef struct PLOTMATRIX
	{
		int xtrans;
		int ytrans;
		int ztrans;
		int alpha;
		float zrot;
	} PLOTMATRIX;

	UITYPE_ANIM AnimType;
	DWORD dwStartTick;
	DWORD dwDuration;
	int iBufferStart;
	int iBufferEnd;
	union
	{
		struct 
		{
			COLORREF clrBack;
			COLORREF clrKey;
			RECT rcFrom;
			PLOTMATRIX mFrom;
			INTERPOLATETYPE iInterpolate;
		} plot;
	} data;

	Control* pOwner;
	CEventSource onAnimationFinished;
};

class AnimationSpooler
{
public:
	AnimationSpooler();
	~AnimationSpooler();

	enum { MAX_BUFFERS = 80 };

	bool init(HWND hWnd);
	bool prepareAnimation(HWND hWnd);
	void cancelJobs();
	bool render();

	bool isAnimating() const { return m_bIsAnimating; }
	bool isJobScheduled() const;
	void addJob(Animation* pJob);

protected:
	COLORREF translateColor(LPDIRECT3DSURFACE9 pSurface, COLORREF clr) const;
	bool setColorKey(LPDIRECT3DTEXTURE9 pTexture, LPDIRECT3DSURFACE9 pSurface, int iTexSize, COLORREF clrColorKey);

	bool prepareJob_Flat(Animation* pJob);
	bool renderJob_Flat(const Animation* pJob, LPDIRECT3DSURFACE9 pSurface, DWORD dwTick);

	struct CUSTOMVERTEX
	{
		FLOAT x, y, z;
		FLOAT rhw;
		DWORD color;
		FLOAT tu, tv;
	};
	typedef CUSTOMVERTEX CUSTOMFAN[4];

	HWND m_hWnd;
	bool m_bIsAnimating;
	bool m_bIsInitialized;
	Array<Animation*> m_aJobs;
	D3DFORMAT m_ColorFormat;
	LPDIRECT3D9 m_pD3D;
	LPDIRECT3DDEVICE9 m_p3DDevice;
	LPDIRECT3DSURFACE9 m_p3DBackSurface;
	LPDIRECT3DVERTEXBUFFER9 m_p3DVertices[MAX_BUFFERS];
	LPDIRECT3DTEXTURE9 m_p3DTextures[MAX_BUFFERS];
	CUSTOMFAN m_fans[MAX_BUFFERS];
	int m_nBuffers;
};

}

#endif // ZGUI_USE_ANIMATION

#endif // __ZGUI_ANIMATION_H_
