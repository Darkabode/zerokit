#include "zgui.h"
#include <dshow.h>

#ifdef ZGUI_USE_WEBCAMERA

namespace zgui
{
    interface ISampleGrabberCB : public IUnknown
    {
        virtual STDMETHODIMP SampleCB (double, IMediaSample*) = 0;
        virtual STDMETHODIMP BufferCB (double, BYTE*, long) = 0;
    };

    interface ISampleGrabber : public IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE SetOneShot (BOOL) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetMediaType (const AM_MEDIA_TYPE*) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType (AM_MEDIA_TYPE*) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetBufferSamples (BOOL) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer (long*, long*) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetCurrentSample (IMediaSample**) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetCallback (ISampleGrabberCB*, long) = 0;
    };

    static const IID IID_ISampleGrabberCB  = { 0x0579154A, 0x2B53, 0x4994, { 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85 } };
    static const IID IID_ISampleGrabber    = { 0x6B652FFF, 0x11FE, 0x4fce, { 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F } };
    static const CLSID CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };
    static const CLSID CLSID_NullRenderer  = { 0xC1F400A4, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

    template <class ComClass>
    class ComBaseClassHelperBase   : public ComClass
    {
    public:
        ComBaseClassHelperBase()  : refCount (1) {}
        virtual ~ComBaseClassHelperBase() {}

        ULONG __stdcall AddRef()
        {
            return ++refCount;
        }
        ULONG __stdcall Release()
        {
            const ULONG r = --refCount;
            if (r == 0) {
                delete this;
            }
            
            return r;
        }

        void resetReferenceCount() throw()     { refCount = 0; }

    protected:
        ULONG refCount;
    };

    class DShowCameraDeviceInteral
    {
    public:
        DShowCameraDeviceInteral (WebCamera* const owner_, const ComSmartPtr <ICaptureGraphBuilder2>& captureGraphBuilder_, const ComSmartPtr <IBaseFilter>& filter_, int minWidth, int minHeight, int maxWidth, int maxHeight) :
        owner(owner_),
        captureGraphBuilder(captureGraphBuilder_),
        filter(filter_),
        ok(false),
        imageNeedsFlipping(false),
        _width(0),
        _height(0),
        activeUsers(0)
        {
            HRESULT hr = graphBuilder.CoCreateInstance (CLSID_FilterGraph);
			if (FAILED(hr)) {
				return;
			}

            hr = captureGraphBuilder->SetFiltergraph (graphBuilder);
			if (FAILED(hr)) {
				return;
			}

            hr = graphBuilder.QueryInterface (mediaControl);
			if (FAILED(hr)) {
				return;
			}

            {
                ComSmartPtr <IAMStreamConfig> streamConfig;

                hr = captureGraphBuilder->FindInterface (&PIN_CATEGORY_CAPTURE, 0, filter,
                    IID_IAMStreamConfig, (void**) streamConfig.resetAndGetPointerAddress());

                if (streamConfig != 0)
                {
                    getVideoSizes (streamConfig);

                    if (! selectVideoSize (streamConfig, minWidth, minHeight, maxWidth, maxHeight))
                        return;
                }
            }

            hr = graphBuilder->AddFilter (filter, L"Video Capture");
            if (FAILED (hr))
                return;

            hr = smartTee.CoCreateInstance (CLSID_SmartTee);
            if (FAILED (hr))
                return;

            hr = graphBuilder->AddFilter (smartTee, L"Smart Tee");
            if (FAILED (hr))
                return;

            if (! connectFilters (filter, smartTee))
                return;

            ComSmartPtr<IBaseFilter> sampleGrabberBase;
            hr = sampleGrabberBase.CoCreateInstance (CLSID_SampleGrabber);
			if (FAILED(hr)) {
				return;
			}

            hr = sampleGrabberBase.QueryInterface (IID_ISampleGrabber, sampleGrabber);
			if (FAILED(hr)) {
				return;
			}

            {
                AM_MEDIA_TYPE mt;
                __stosb((uint8_t*)&mt, 0, sizeof(mt));
                mt.majortype = MEDIATYPE_Video;
                mt.subtype = MEDIASUBTYPE_RGB24;
                mt.formattype = FORMAT_VideoInfo;
                sampleGrabber->SetMediaType (&mt);
            }

            _callback = new GrabberCallback(*this);
            hr = sampleGrabber->SetCallback (_callback, 1);

            hr = graphBuilder->AddFilter(sampleGrabberBase, L"Sample Grabber");
			if (FAILED(hr)) {
				return;
			}

            ComSmartPtr <IPin> grabberInputPin;
            if (!(getPin(smartTee, PINDIR_OUTPUT, smartTeeCaptureOutputPin, L"capture") && getPin(smartTee, PINDIR_OUTPUT, smartTeePreviewOutputPin, L"preview")
                && getPin (sampleGrabberBase, PINDIR_INPUT, grabberInputPin))) {
                return;
            }

            hr = graphBuilder->Connect(smartTeePreviewOutputPin, grabberInputPin);
            if (FAILED(hr)) {
                return;
            }

            AM_MEDIA_TYPE mt;
            __stosb((uint8_t*)&mt, 0, sizeof(mt));
            hr = sampleGrabber->GetConnectedMediaType(&mt);
            VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)(mt.pbFormat);
            _width = pVih->bmiHeader.biWidth;
            _height = pVih->bmiHeader.biHeight;

            ComSmartPtr<IBaseFilter> nullFilter;
            hr = nullFilter.CoCreateInstance(CLSID_NullRenderer);
            hr = graphBuilder->AddFilter(nullFilter, L"Null Renderer");

            if (connectFilters(sampleGrabberBase, nullFilter) && addGraphToRot()) {
                hActiveImage = createImage(_width, _height, &pActiveBits);
                hLoadingImage = createImage(_width, _height, &pLoadingBits);

                ok = true;
            }
        }

        ~DShowCameraDeviceInteral()
        {
            if (mediaControl != 0) {
                mediaControl->Stop();
            }

            removeGraphFromRot();

            delete _callback;
            graphBuilder = 0;
            sampleGrabber = 0;
            mediaControl = 0;
            filter = 0;
            captureGraphBuilder = 0;
            smartTee = 0;
            smartTeePreviewOutputPin = 0;
            smartTeeCaptureOutputPin = 0;

            fn_DeleteObject(hActiveImage);
            fn_DeleteObject(hLoadingImage);
        }

        HBITMAP createImage(int width, int height, uint8_t** pBits)
        {
            BITMAPINFO bmi;
            
            bmi.bmiHeader.biSize = sizeof(bmi);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = height;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 24;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage = 0;
            bmi.bmiHeader.biXPelsPerMeter = 0;
            bmi.bmiHeader.biYPelsPerMeter = 0;
            bmi.bmiHeader.biClrUsed = 0;
            bmi.bmiHeader.biClrImportant = 0;

            return fn_CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)pBits, NULL, 0);
        }

        void addUser()
        {
            if (ok && activeUsers++ == 0) {
                mediaControl->Run();
            }
        }

        void removeUser()
        {
            if (ok && --activeUsers == 0) {
                mediaControl->Stop();
            }
        }

        void handleFrame (double /*time*/, BYTE* buffer, long /*bufferSize*/)
        {
            {
                const int lineStride = _width * 3;
                const ScopedLock sl (imageSwapLock);

                __movsb(pLoadingBits, buffer, lineStride * _height);
                imageNeedsFlipping = true;
            }

            owner->Invalidate();
        }

        void drawCurrentImage(HDC hDC,int x, int y, int w, int h)
        {
            if (imageNeedsFlipping) {
                const ScopedLock sl (imageSwapLock);
                uint8_t* pTempBits = pLoadingBits;
                HBITMAP hTempBitmap = hLoadingImage;
                hLoadingImage = hActiveImage;
                hActiveImage = hTempBitmap;
                pLoadingBits = pActiveBits;
                pActiveBits = pTempBits;
                imageNeedsFlipping = false;
            }
 
            HDC hCloneDC = fn_CreateCompatibleDC(hDC);
            HBITMAP hOldBitmap = (HBITMAP)fn_SelectObject(hCloneDC, hActiveImage);
            fn_SetStretchBltMode(hDC, HALFTONE);

            fn_StretchBlt(hDC, x, y, w, h, hCloneDC, 0, 0, _width, _height, SRCCOPY);

            fn_SelectObject(hCloneDC, hOldBitmap);
            fn_DeleteDC(hCloneDC);
        }

        bool ok;
        int _width, _height;

    private:
        WebCamera* const owner;
        ComSmartPtr <ICaptureGraphBuilder2> captureGraphBuilder;
        ComSmartPtr <IBaseFilter> filter;
        ComSmartPtr <IBaseFilter> smartTee;
        ComSmartPtr <IGraphBuilder> graphBuilder;
        ComSmartPtr <ISampleGrabber> sampleGrabber;
        ComSmartPtr <IMediaControl> mediaControl;
        ComSmartPtr <IPin> smartTeePreviewOutputPin;
        ComSmartPtr <IPin> smartTeeCaptureOutputPin;
        int activeUsers;
        Array<int> widths;
		Array<int> heights;
        DWORD graphRegistrationID;

        CriticalSection imageSwapLock;
        bool imageNeedsFlipping;
        HBITMAP hLoadingImage, hActiveImage;
        uint8_t* pLoadingBits;
        uint8_t* pActiveBits;

        void getVideoSizes (IAMStreamConfig* const streamConfig)
        {
			widths.clear();
			heights.clear();
            
            int count = 0, size = 0;
            streamConfig->GetNumberOfCapabilities (&count, &size);

            if (size == sizeof (VIDEO_STREAM_CONFIG_CAPS)) {
                for (int i = 0; i < count; ++i) {
                    VIDEO_STREAM_CONFIG_CAPS scc;
                    AM_MEDIA_TYPE* config;

                    HRESULT hr = streamConfig->GetStreamCaps (i, &config, (BYTE*) &scc);

                    if (SUCCEEDED(hr)) {
                        const int w = scc.InputSize.cx;
                        const int h = scc.InputSize.cy;

                        bool duplicate = false;

                        for (int j = 0, count = widths.size(); j < count; ++j) {
                            if (w == widths.getUnchecked(j) && h == heights.getUnchecked(j)) {
                                duplicate = true;
                                break;
                            }
                        }

                        if (!duplicate) {
                            //DBG ("Camera capture size: " + String (w) + ", " + String (h));
							widths.add(w);
                            heights.add (h);
                        }

                        deleteMediaType (config);
                    }
                }
            }
        }

        bool selectVideoSize (IAMStreamConfig* const streamConfig, const int minWidth, const int minHeight, const int maxWidth, const int maxHeight)
        {
            int count = 0, size = 0, bestArea = 0, bestIndex = -1;
            streamConfig->GetNumberOfCapabilities (&count, &size);

            if (size == sizeof (VIDEO_STREAM_CONFIG_CAPS)) {
                AM_MEDIA_TYPE* config;
                VIDEO_STREAM_CONFIG_CAPS scc;

                for (int i = 0; i < count; ++i) {
                    HRESULT hr = streamConfig->GetStreamCaps (i, &config, (BYTE*) &scc);

                    if (SUCCEEDED(hr)) {
                        if (scc.InputSize.cx >= minWidth && scc.InputSize.cy >= minHeight && scc.InputSize.cx <= maxWidth && scc.InputSize.cy <= maxHeight) {
                            int area = scc.InputSize.cx * scc.InputSize.cy;
                            if (area > bestArea) {
                                bestIndex = i;
                                bestArea = area;
                            }
                        }

                        deleteMediaType(config);
                    }
                }

                if (bestIndex >= 0) {
                    HRESULT hr = streamConfig->GetStreamCaps (bestIndex, &config, (BYTE*) &scc);

                    hr = streamConfig->SetFormat (config);
                    deleteMediaType (config);
                    return SUCCEEDED (hr);
                }
            }

            return false;
        }

        static bool getPin (IBaseFilter* filter, const PIN_DIRECTION wantedDirection, ComSmartPtr<IPin>& result, const wchar_t* pinName = 0)
        {
            ComSmartPtr <IEnumPins> enumerator;
            ComSmartPtr <IPin> pin;

            filter->EnumPins (enumerator.resetAndGetPointerAddress());

            while (enumerator->Next (1, pin.resetAndGetPointerAddress(), 0) == S_OK)
            {
                PIN_DIRECTION dir;
                pin->QueryDirection (&dir);

                if (wantedDirection == dir)
                {
                    PIN_INFO info;
                    __stosb((uint8_t*)&info, 0, sizeof(info));
                    pin->QueryPinInfo (&info);

                    if (pinName == 0 || fn_StrCmpIW(pinName, info.achName) == 0) {
                        result = pin;
                        return true;
                    }
                }
            }

            return false;
        }

        bool connectFilters (IBaseFilter* const first, IBaseFilter* const second) const
        {
            ComSmartPtr <IPin> in, out;

            return getPin (first, PINDIR_OUTPUT, out) && getPin (second, PINDIR_INPUT, in) && SUCCEEDED (graphBuilder->Connect (out, in));
        }

        bool addGraphToRot()
        {
            ComSmartPtr <IRunningObjectTable> rot;
            if (FAILED (fn_GetRunningObjectTable (0, rot.resetAndGetPointerAddress())))
                return false;

            ComSmartPtr <IMoniker> moniker;
            WCHAR buffer[128];
            HRESULT hr = fn_CreateItemMoniker (L"!", buffer, moniker.resetAndGetPointerAddress());
            if (FAILED (hr))
                return false;

            graphRegistrationID = 0;
            return SUCCEEDED (rot->Register (0, graphBuilder, moniker, &graphRegistrationID));
        }

        void removeGraphFromRot()
        {
            ComSmartPtr <IRunningObjectTable> rot;

            if (SUCCEEDED (fn_GetRunningObjectTable (0, rot.resetAndGetPointerAddress())))
                rot->Revoke (graphRegistrationID);
        }

        static void deleteMediaType (AM_MEDIA_TYPE* const pmt)
        {
            if (pmt->cbFormat != 0)
                fn_CoTaskMemFree ((PVOID) pmt->pbFormat);

            if (pmt->pUnk != 0)
                pmt->pUnk->Release();

            fn_CoTaskMemFree (pmt);
        }

        //==============================================================================
        class GrabberCallback : public ComBaseClassHelperBase <ISampleGrabberCB>
        {
        public:
            GrabberCallback (DShowCameraDeviceInteral& owner_)  : owner (owner_) {}

            HRESULT __stdcall QueryInterface (REFIID refId, void** result)
            {
                if (fn_IsEqualGUID(refId, IID_ISampleGrabberCB)) {
                    AddRef();
                    *result = dynamic_cast<ISampleGrabberCB*>(this);
                    return S_OK;
                }
                if (fn_IsEqualGUID(refId, IID_IUnknown)) {
                    AddRef();
                    *result = dynamic_cast<IUnknown*>(this);
                    return S_OK;
                }

                *result = 0;
                return E_NOINTERFACE;
            }

            STDMETHODIMP SampleCB (double /*SampleTime*/, IMediaSample* /*pSample*/)  { return E_FAIL; }

            STDMETHODIMP BufferCB (double time, BYTE* buffer, long bufferSize)
            {
                owner.handleFrame (time, buffer, bufferSize);
                return S_OK;
            }
        private:
            DShowCameraDeviceInteral& owner;
        };

        GrabberCallback* _callback;
        CriticalSection listenerLock;
    };


ComSmartPtr<IBaseFilter> enumerateCameras(StringArray* names, const int deviceIndexToOpen, String& name)
{
    int index = 0;
    ComSmartPtr <IBaseFilter> result;

    ComSmartPtr <ICreateDevEnum> pDevEnum;
    HRESULT hr = pDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);

    if (SUCCEEDED(hr)) {
        ComSmartPtr <IEnumMoniker> enumerator;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, enumerator.resetAndGetPointerAddress(), 0);

        if (SUCCEEDED (hr) && enumerator != 0) {
            ComSmartPtr <IMoniker> moniker;
            ULONG fetched;

            while (enumerator->Next(1, moniker.resetAndGetPointerAddress(), &fetched) == S_OK) {
                ComSmartPtr<IBaseFilter> captureFilter;
                hr = moniker->BindToObject(0, 0, IID_IBaseFilter, (void**) captureFilter.resetAndGetPointerAddress());

                if (SUCCEEDED(hr)) {
                    ComSmartPtr <IPropertyBag> propertyBag;
                    hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**) propertyBag.resetAndGetPointerAddress());

                    if (SUCCEEDED(hr)) {
                        VARIANT var;
                        var.vt = VT_BSTR;

                        hr = propertyBag->Read(L"FriendlyName", &var, 0);
                        propertyBag = 0;

                        if (SUCCEEDED (hr)) {
                            if (names != 0) {
                                names->add(String(var.bstrVal));
                                //names->add (var.bstrVal);
                            }

                            if (index == deviceIndexToOpen) {
                                name = var.bstrVal;
                                result = captureFilter;
                                break;
                            }

                            ++index;
                        }
                    }
                }
            }
        }
    }

    return result;
}

const String WebCamera::CLASS_NAME = "WebCam";

WebCamera::WebCamera() :
_owner(0)
{
    
}

WebCamera::~WebCamera()
{
    if (_owner != 0) {
        _owner->removeUser();
        delete _owner;
    }
}

const String& WebCamera::getClass() const
{
    return CLASS_NAME;
}

void WebCamera::enumerate(StringArray* pCameras)
{
    String dummy;
    enumerateCameras(pCameras, -1, dummy);
}

void WebCamera::startCapture(int index /* = 1 */)
{
    ComSmartPtr <ICaptureGraphBuilder2> captureGraphBuilder;
    HRESULT hr = captureGraphBuilder.CoCreateInstance (CLSID_CaptureGraphBuilder2);
 
    if (SUCCEEDED(hr)) {
        String name;
        const ComSmartPtr<IBaseFilter> filter(enumerateCameras(0, index, name));

        if (filter != 0) {
            _name = name;

            _owner = new DShowCameraDeviceInteral(this, captureGraphBuilder, filter, GetFixedWidth(), GetFixedHeight(), 2 * GetFixedWidth(), 2 * GetFixedHeight());

            if (_owner->ok) {
                _owner->addUser();
            }
            else {
                delete _owner;
                _owner = 0;
            }
        }
    }
}

void WebCamera::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if (_owner != 0) {
        _owner->drawCurrentImage(hDC, GetX(), GetY(), GetFixedWidth(), GetFixedHeight());
    }
//     else {
// //         Gdiplus::SolidBrush brush(Gdiplus::Color::Black);
// //         g.FillRectangle(&brush, GetX(), GetY(), GetFixedWidth(), GetFixedHeight());
//     }
}

}

#endif // ZGUI_USE_WEBCAMERA