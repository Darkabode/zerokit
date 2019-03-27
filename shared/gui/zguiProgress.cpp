#include "zgui.h"

#ifdef ZGUI_USE_PROGRESS

namespace zgui {

const String Progress::CLASS_NAME = ZGUI_PROGRESS;

Progress::Progress() :
_bHorizontal(true),
_min(0),
_max(100),
_value(0),
_foreInset(0),
_bStretchForeImage(true)
{
	_uTextStyle = DT_SINGLELINE | DT_CENTER;
	SetFixedHeight(12);
}

const String& Progress::getClass() const
{
	return CLASS_NAME;
}

LPVOID Progress::getInterface(const String& name)
{
    if (name == ZGUI_PROGRESS) {
        return static_cast<Progress*>(this);
    }
	return Label::getInterface(name);
}

bool Progress::IsHorizontal()
{
	return _bHorizontal;
}

void Progress::SetHorizontal(bool bHorizontal)
{
	if (_bHorizontal == bHorizontal) {
		return;
	}

	_bHorizontal = bHorizontal;
	Invalidate();
}

int Progress::GetMinValue() const
{
	return _min;
}

void Progress::SetMinValue(int nMin)
{
	_min = nMin;
	Invalidate();
}

int Progress::GetMaxValue() const
{
	return _max;
}

void Progress::SetMaxValue(int nMax)
{
	_max = nMax;
	Invalidate();
}

int Progress::GetValue() const
{
	return _value;
}

void Progress::SetValue(int nValue)
{
	_value = nValue;
	Invalidate();
}

const String& Progress::GetForeImage() const
{
	return _sForeImage;
}

void Progress::setForeImage(const String& strImage)
{
	if (_sForeImage != strImage) {
		_sForeImage = strImage;
		Invalidate();
	}
}

void Progress::setForeInset(int inset)
{
	_foreInset = inset;
}

void Progress::setAttribute(const String& name, const String& value)
{
    if (name == "foreimage") {
        setForeImage(value);
    }
	else if (name == "foreinset") {
		setForeInset(value.getIntValue());
	}
    else if (name == "hor") {
        SetHorizontal(value == "true");
    }
    else if (name == "min") {
        SetMinValue(value.getIntValue());
    }
    else if (name == "max") {
        SetMaxValue(value.getIntValue());
    }
    else if (name == "value") {
        SetValue(value.getIntValue());
    }
    else if (name == "isstretchfore") {
        SetStretchForeImage(value == "true" ? true : false);
    }
    else {
        Label::setAttribute(name, value);
    }
}

void Progress::PaintStatusImage(HDC hDC)
{
	if (_max <= _min) {
		_max = _min + 1;
	}
	if (_value > _max) {
		_value = _max;
	}
	if (_value < _min) {
		_value = _min;
	}

	RECT rc;
	__stosb((uint8_t*)&rc, 0, sizeof(rc));
	if (_bHorizontal) {
		rc.right = (_value - _min) * (_rcItem.right - _rcItem.left) / (_max - _min);
		rc.bottom = _rcItem.bottom - _rcItem.top;
	}
	else {
		rc.top = (_rcItem.bottom - _rcItem.top) * (_max - _value) / (_max - _min);
		rc.right = _rcItem.right - _rcItem.left;
		rc.bottom = _rcItem.bottom - _rcItem.top;
	}

	if (!_sForeImage.isEmpty()) {
// 		_sForeImageModify = String::empty;
// 		if (_bStretchForeImage) {
// 			_sForeImageModify = String::formatted("dest='%d,%d,%d,%d'", rc.left, rc.top, rc.right, rc.bottom);
// 		}
// 		else {
// 			_sForeImageModify = String::formatted("dest='%d,%d,%d,%d' source='%d,%d,%d,%d'", rc.left, rc.top, rc.right, rc.bottom, rc.left, rc.top, rc.right, rc.bottom);
// 		}
		if (!DrawImage(hDC, _sForeImage + String::formatted(" dest='%d,%d,%d,%d'", rc.left + _foreInset, rc.top, rc.right - _foreInset, rc.bottom), String::empty/*_sForeImageModify*/)) {
			_sForeImage = String::empty;
		}
	}
}

bool Progress::IsStretchForeImage()
{
	return _bStretchForeImage;
}

void Progress::SetStretchForeImage(bool bStretchForeImage)
{
	if (_bStretchForeImage == bStretchForeImage) {
		return;
	}

	_bStretchForeImage = bStretchForeImage;
	Invalidate();
}

}

#endif // ZGUI_USE_PROGRESS
