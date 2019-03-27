#ifndef __ZGUI_CAMERA_H_
#define __ZGUI_CAMERA_H_

namespace zgui
{

class DShowCameraDeviceInteral;

class Camera : public CControlUI
{
public:
    Camera();
    ~Camera();
    
    LPCTSTR GetClass() const;

    static void enumerate(StringArray* pCameras);

    void startCapture(int index = 0);
    
protected:
    virtual void DoPaint(HDC hDC, const RECT& rcPaint);

private:
    void openDevice(int index, int minWidth, int minHeight, int maxWidth, int maxHeight);

    String _name;
    DShowCameraDeviceInteral* _owner;
};

}

#endif // __ZGUI_CAMERA_H_
