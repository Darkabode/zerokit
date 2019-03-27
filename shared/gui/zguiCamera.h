#ifndef __ZGUI_CAMERA_H_
#define __ZGUI_CAMERA_H_

#ifdef ZGUI_USE_WEBCAMERA

namespace zgui
{

class DShowCameraDeviceInteral;

class WebCamera : public Control
{
public:
    WebCamera();
    ~WebCamera();
    
    const String& getClass() const;

    static void enumerate(StringArray* pCameras);

    void startCapture(int index = 0);
    
protected:
    virtual void DoPaint(HDC hDC, const RECT& rcPaint);

private:
    void openDevice(int index, int minWidth, int minHeight, int maxWidth, int maxHeight);

    String _name;
    DShowCameraDeviceInteral* _owner;

private:
	static const String CLASS_NAME;
};

}

#endif // ZGUI_USE_WEBCAMERA

#endif // __ZGUI_CAMERA_H_
