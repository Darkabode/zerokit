#include "zgui.h"
#include <dshow.h>

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

    /** A simple COM smart pointer. */
    template <class ComClass>
    class ComSmartPtr
    {
    public:
        ComSmartPtr() throw() :
        p(0)
        {
        }

        ComSmartPtr(ComClass* const p_) :
        p(p_)
        {
            if (p_ != 0) {
                p_->AddRef();
            }
        }
        
        ComSmartPtr(const ComSmartPtr<ComClass>& p_) :
        p(p_.p)
        {
            if (p != 0) {
                p->AddRef();
            }
        }
        
        ~ComSmartPtr()
        {
            release();
        }

        operator ComClass*() const throw()
        {
            return p;
        }

        ComClass& operator*() const throw()
        {
            return *p;
        }

        ComClass* operator->() const throw()
        {
            return p;
        }

        ComSmartPtr& operator= (ComClass* const newP)
        {
            if (newP != 0) {
                newP->AddRef();
            }
            release();
            p = newP;
            return *this;
        }

        ComSmartPtr& operator= (const ComSmartPtr<ComClass>& newP)  { return operator= (newP.p); }

        // Releases and nullifies this pointer and returns its address
        ComClass** resetAndGetPointerAddress()
        {
            release();
            p = 0;
            return &p;
        }

        HRESULT CoCreateInstance (REFCLSID classUUID, DWORD dwClsContext = CLSCTX_INPROC_SERVER)
        {
            return ::CoCreateInstance (classUUID, 0, dwClsContext, __uuidof (ComClass), (void**) resetAndGetPointerAddress());
        }

        template <class OtherComClass>
        HRESULT QueryInterface (REFCLSID classUUID, ComSmartPtr<OtherComClass>& destObject) const
        {
            if (p == 0) {
                return E_POINTER;
            }

            return p->QueryInterface (classUUID, (void**) destObject.resetAndGetPointerAddress());
        }

        template <class OtherComClass>
        HRESULT QueryInterface (ComSmartPtr<OtherComClass>& destObject) const
        {
            return this->QueryInterface (__uuidof (OtherComClass), destObject);
        }

    private:
        ComClass* p;
 
        void release()
        {
            if (p != 0) {
                p->Release();
            }
        }

        ComClass** operator&() throw(); // private to avoid it being used accidentally
    };


    class DShowCameraDeviceInteral
    {
    public:
        DShowCameraDeviceInteral (Camera* const owner_, const ComSmartPtr <ICaptureGraphBuilder2>& captureGraphBuilder_,
            const ComSmartPtr <IBaseFilter>& filter_, int minWidth, int minHeight, int maxWidth, int maxHeight) :
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
            if (FAILED (hr))
                return;

            hr = captureGraphBuilder->SetFiltergraph (graphBuilder);
            if (FAILED (hr))
                return;

            hr = graphBuilder.QueryInterface (mediaControl);
            if (FAILED (hr))
                return;

            cvec_init(widths);
            cvec_init(heights);

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

            hr = graphBuilder->AddFilter (filter, _T("Video Capture"));
            if (FAILED (hr))
                return;

            hr = smartTee.CoCreateInstance (CLSID_SmartTee);
            if (FAILED (hr))
                return;

            hr = graphBuilder->AddFilter (smartTee, _T("Smart Tee"));
            if (FAILED (hr))
                return;

            if (! connectFilters (filter, smartTee))
                return;

            ComSmartPtr <IBaseFilter> sampleGrabberBase;
            hr = sampleGrabberBase.CoCreateInstance (CLSID_SampleGrabber);
            if (FAILED (hr))
                return;

            hr = sampleGrabberBase.QueryInterface (IID_ISampleGrabber, sampleGrabber);
            if (FAILED (hr))
                return;

            {
                AM_MEDIA_TYPE mt = { 0 };
                mt.majortype = MEDIATYPE_Video;
                mt.subtype = MEDIASUBTYPE_RGB24;
                mt.formattype = FORMAT_VideoInfo;
                sampleGrabber->SetMediaType (&mt);
            }

            _callback = new GrabberCallback(*this);
            hr = sampleGrabber->SetCallback (_callback, 1);

            hr = graphBuilder->AddFilter(sampleGrabberBase, _T("Sample Grabber"));
            if (FAILED (hr))
                return;

            ComSmartPtr <IPin> grabberInputPin;
            if (!(getPin(smartTee, PINDIR_OUTPUT, smartTeeCaptureOutputPin, L"capture") && getPin(smartTee, PINDIR_OUTPUT, smartTeePreviewOutputPin, L"preview")
                && getPin (sampleGrabberBase, PINDIR_INPUT, grabberInputPin))) {
                return;
            }

            hr = graphBuilder->Connect(smartTeePreviewOutputPin, grabberInputPin);
            if (FAILED(hr)) {
                return;
            }

            AM_MEDIA_TYPE mt = { 0 };
            hr = sampleGrabber->GetConnectedMediaType(&mt);
            VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)(mt.pbFormat);
            _width = pVih->bmiHeader.biWidth;
            _height = pVih->bmiHeader.biHeight;

            ComSmartPtr<IBaseFilter> nullFilter;
            hr = nullFilter.CoCreateInstance(CLSID_NullRenderer);
            hr = graphBuilder->AddFilter(nullFilter, _T("Null Renderer"));

            if (connectFilters(sampleGrabberBase, nullFilter) && addGraphToRot()) {
                pActiveImage = new Gdiplus::Bitmap(_width, _height, PixelFormat24bppRGB);
                pLoadingImage = new Gdiplus::Bitmap(_width, _height, PixelFormat24bppRGB);

                ok = true;
            }
        }

        ~DShowCameraDeviceInteral()
        {
            if (mediaControl != 0) {
                mediaControl->Stop();
            }

            removeGraphFromRot();

            _callback = 0;
            graphBuilder = 0;
            sampleGrabber = 0;
            mediaControl = 0;
            filter = 0;
            captureGraphBuilder = 0;
            smartTee = 0;
            smartTeePreviewOutputPin = 0;
            smartTeeCaptureOutputPin = 0;

            delete pActiveImage;
            delete pLoadingImage;

            cvec_destroy(widths);
            cvec_destroy(heights);
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

                {
				    Gdiplus::BitmapData descData;
				    Gdiplus::Rect rc(0, 0, _width, _height);
				    pLoadingImage->LockBits(&rc, Gdiplus::ImageLockModeWrite, PixelFormat24bppRGB, &descData);
                    for (int i = 0; i < _height; ++i) {
                        memcpy ((uint8_t*)descData.Scan0 + descData.Stride * ((_height - 1) - i), buffer + lineStride * i, descData.Stride);
                    }
				    pLoadingImage->UnlockBits(&descData);
                }

                imageNeedsFlipping = true;
            }

            owner->Invalidate();
        }

        void drawCurrentImage(Gdiplus::Graphics& g, int x, int y, int w, int h)
        {
            if (imageNeedsFlipping) {
                const ScopedLock sl (imageSwapLock);
                Gdiplus::Bitmap* pTemp = pLoadingImage;
                pLoadingImage = pActiveImage;
                pActiveImage = pTemp;
                imageNeedsFlipping = false;
            }

            Gdiplus::Rect destRect(x, y, w, h);
            g.DrawImage(pActiveImage, destRect, 0, 0, _width, _height, Gdiplus::UnitPixel);
        }

        bool ok;
        int _width, _height;

    private:
        Camera* const owner;
        ComSmartPtr <ICaptureGraphBuilder2> captureGraphBuilder;
        ComSmartPtr <IBaseFilter> filter;
        ComSmartPtr <IBaseFilter> smartTee;
        ComSmartPtr <IGraphBuilder> graphBuilder;
        ComSmartPtr <ISampleGrabber> sampleGrabber;
        ComSmartPtr <IMediaControl> mediaControl;
        ComSmartPtr <IPin> smartTeePreviewOutputPin;
        ComSmartPtr <IPin> smartTeeCaptureOutputPin;
        int activeUsers;
        cvec_t(int) widths;
        cvec_t(int) heights;
        DWORD graphRegistrationID;

        CriticalSection imageSwapLock;
        bool imageNeedsFlipping;
        Gdiplus::Bitmap* pLoadingImage;
        Gdiplus::Bitmap* pActiveImage;

        void getVideoSizes (IAMStreamConfig* const streamConfig)
        {
            cvec_size(widths) = 0;
            cvec_size(heights) = 0;

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

                        for (int j = 0, count = cvec_size(widths); j < count; ++j) {
                            
                            if (w == cvec_A(widths, j) && h == cvec_A(heights, j)) {
                                duplicate = true;
                                break;
                            }
                        }

                        if (!duplicate) {
                            //DBG ("Camera capture size: " + String (w) + ", " + String (h));
                            cvec_push(int, widths, w);
                            //widths.add (w);
                            cvec_push(int, heights, h);
                            //heights.add (h);
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
                    PIN_INFO info = { 0 };
                    pin->QueryPinInfo (&info);

                    if (pinName == 0 || lstrcmpi(pinName, info.achName) == 0) {
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
            if (FAILED (GetRunningObjectTable (0, rot.resetAndGetPointerAddress())))
                return false;

            ComSmartPtr <IMoniker> moniker;
            WCHAR buffer[128];
            HRESULT hr = CreateItemMoniker (_T("!"), buffer, moniker.resetAndGetPointerAddress());
            if (FAILED (hr))
                return false;

            graphRegistrationID = 0;
            return SUCCEEDED (rot->Register (0, graphBuilder, moniker, &graphRegistrationID));
        }

        void removeGraphFromRot()
        {
            ComSmartPtr <IRunningObjectTable> rot;

            if (SUCCEEDED (GetRunningObjectTable (0, rot.resetAndGetPointerAddress())))
                rot->Revoke (graphRegistrationID);
        }

        static void deleteMediaType (AM_MEDIA_TYPE* const pmt)
        {
            if (pmt->cbFormat != 0)
                CoTaskMemFree ((PVOID) pmt->pbFormat);

            if (pmt->pUnk != 0)
                pmt->pUnk->Release();

            CoTaskMemFree (pmt);
        }

        //==============================================================================
        class GrabberCallback : public ComBaseClassHelperBase <ISampleGrabberCB>
        {
        public:
            GrabberCallback (DShowCameraDeviceInteral& owner_)  : owner (owner_) {}

            HRESULT __stdcall QueryInterface (REFIID refId, void** result)
            {
                if (refId == IID_ISampleGrabberCB)  { AddRef(); *result = dynamic_cast <ISampleGrabberCB*> (this); return S_OK; }
                if (refId == IID_IUnknown)          { AddRef(); *result = dynamic_cast <IUnknown*> (this); return S_OK; }

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

        ComSmartPtr <GrabberCallback> _callback;
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

                        hr = propertyBag->Read(_T("FriendlyName"), &var, 0);
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

Camera::Camera() :
_owner(0)
{
    
}

Camera::~Camera()
{
    if (_owner != 0) {
        _owner->removeUser();
        delete _owner;
    }
}

LPCTSTR Camera::GetClass() const
{
    return _T("Camera");
}

void Camera::enumerate(StringArray* pCameras)
{
    String dummy;
    enumerateCameras(pCameras, -1, dummy);
}

void Camera::startCapture(int index /* = 1 */)
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

void Camera::DoPaint(HDC hDC, const RECT& rcPaint)
{
    Gdiplus::Graphics g(hDC);
    g.SetInterpolationMode(Gdiplus::InterpolationModeDefault);

    if (_owner != 0) {
        _owner->drawCurrentImage(g, GetX(), GetY(), GetFixedWidth(), GetFixedHeight());
    }
    else {
        Gdiplus::SolidBrush brush(Gdiplus::Color::Black);
        g.FillRectangle(&brush, GetX(), GetY(), GetFixedWidth(), GetFixedHeight());
    }
}

}
