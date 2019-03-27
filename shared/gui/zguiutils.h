#ifndef __ZGUI_UTILS_H_
#define __ZGUI_UTILS_H_

namespace zgui {

int isalnum(int c);
int isdigit(int c);

class Point : public POINT
{
public:
	Point();
	Point(const POINT& src);
	Point(int x, int y);
	Point(LPARAM lParam);
};

class Size : public SIZE
{
public:
	Size();
	Size(const SIZE& src);
	Size(const RECT rc);
	Size(int cx, int cy);
};

struct Helper
{
    static bool splitString(const String& str, const String& seps, const String& quotes, int& val1, int& val2, int& val3, int& val4);
    static bool splitString(const String& str, const String& seps, const String& quotes, int& val1, int& val2);
};

class Rect : public RECT
{
public:
	Rect();
	Rect(const RECT& src);
	Rect(int iLeft, int iTop, int iRight, int iBottom);

	int getWidth() const;
	int getHeight() const;
	void empty();
	bool isNull() const;
	void joinWith(const RECT& rc);
	void resetOffset();
	void normalize();
	void offsetWith(int cx, int cy);
	void inflateWith(int cx, int cy);
	void deflateWith(int cx, int cy);
	void unionWith(Rect& rc);
};

class WaitCursor
{
public:
	WaitCursor();
	~WaitCursor();

protected:
	HCURSOR m_hOrigCursor;
};

} // namespace zgui

#endif // __ZGUI_UTILS_H_