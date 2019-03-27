#ifndef __ZGUI_PROGRESS_H_
#define __ZGUI_PROGRESS_H_

#ifdef ZGUI_USE_PROGRESS

namespace zgui {

class Progress : public Label
{
public:
	Progress();

	const String& getClass() const;
	LPVOID getInterface(const String& name);

	bool IsHorizontal();
	void SetHorizontal(bool bHorizontal = true);
	bool IsStretchForeImage();
	void SetStretchForeImage(bool bStretchForeImage = true);
	int GetMinValue() const;
	void SetMinValue(int nMin);
	int GetMaxValue() const;
	void SetMaxValue(int nMax);
	int GetValue() const;
	void SetValue(int nValue);
	const String& GetForeImage() const;
	void setForeImage(const String& pStrImage);
	void setForeInset(int inset);

	void setAttribute(const String& pstrName, const String& pstrValue);
	void PaintStatusImage(HDC hDC);

protected:
	bool _bHorizontal;
	bool _bStretchForeImage;
	int _max;
	int _min;
	int _value;
	int _foreInset;

	String _sForeImage;
	String _sForeImageModify;

private:
	static const String CLASS_NAME;
};

} // namespace zgui

#endif // ZGUI_USE_PROGRESS

#endif // __ZGUI_PROGRESS_H_
