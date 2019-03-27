namespace zgui
{
    int isalnum(int c)
    {
        return ((c<=L'z' && c>=L'a') || (c<=L'Z' && c>=L'A') || (c<=L'9' && c>=L'0'));
    }

    int isdigit(int c)
    {
        return (c <= L'9' && c >= L'0');
    }


	Point::Point()
	{
		x = y = 0;
	}

	Point::Point(const POINT& src)
	{
		x = src.x;
		y = src.y;
	}

	Point::Point(int _x, int _y)
	{
		x = _x;
		y = _y;
	}

	Point::Point(LPARAM lParam)
	{
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
	}


	Size::Size()
	{
		cx = cy = 0;
	}

	Size::Size(const SIZE& src)
	{
		cx = src.cx;
		cy = src.cy;
	}

	Size::Size(const RECT rc)
	{
		cx = rc.right - rc.left;
		cy = rc.bottom - rc.top;
	}

	Size::Size(int _cx, int _cy)
	{
		cx = _cx;
		cy = _cy;
	}


	Rect::Rect()
	{
		left = top = right = bottom = 0;
	}

	Rect::Rect(const RECT& src)
	{
		left = src.left;
		top = src.top;
		right = src.right;
		bottom = src.bottom;
	}

	Rect::Rect(int iLeft, int iTop, int iRight, int iBottom)
	{
		left = iLeft;
		top = iTop;
		right = iRight;
		bottom = iBottom;
	}

	int Rect::getWidth() const
	{
		return right - left;
	}

	int Rect::getHeight() const
	{
		return bottom - top;
	}

	void Rect::empty()
	{
		left = top = right = bottom = 0;
	}

	bool Rect::isNull() const
	{
		return (left == 0 && right == 0 && top == 0 && bottom == 0); 
	}

	void Rect::joinWith(const RECT& rc)
	{
		if( rc.left < left ) left = rc.left;
		if( rc.top < top ) top = rc.top;
		if( rc.right > right ) right = rc.right;
		if( rc.bottom > bottom ) bottom = rc.bottom;
	}

	void Rect::resetOffset()
	{
		fn_OffsetRect(this, -left, -top);
	}

	void Rect::normalize()
	{
		if( left > right ) { int iTemp = left; left = right; right = iTemp; }
		if( top > bottom ) { int iTemp = top; top = bottom; bottom = iTemp; }
	}

	void Rect::offsetWith(int cx, int cy)
	{
		fn_OffsetRect(this, cx, cy);
	}

	void Rect::inflateWith(int cx, int cy)
	{
		fn_InflateRect(this, cx, cy);
	}

	void Rect::deflateWith(int cx, int cy)
	{
		fn_InflateRect(this, -cx, -cy);
	}

	void Rect::unionWith(Rect& rc)
	{
		fn_UnionRect(this, this, &rc);
	}


    bool Helper::splitString(const String& str, const String& seps, const String& quotes, int& val1, int& val2, int& val3, int& val4)
    {
        StringArray params;
        params.addTokens(str, seps, quotes);
        if (params.size() != 4) {
            return false;
        }

        val1 = params[0].getIntValue();
        val2 = params[1].getIntValue();
        val3 = params[2].getIntValue();
        val4 = params[3].getIntValue();

        return true;
    }

    bool Helper::splitString(const String& str, const String& seps, const String& quotes, int& val1, int& val2)
    {
        StringArray params;
        params.addTokens(str, seps, quotes);
        if (params.size() != 2) {
            return false;
        }

        val1 = params[0].getIntValue();
        val2 = params[1].getIntValue();

        return true;
    }

	WaitCursor::WaitCursor()
	{
		m_hOrigCursor = fn_SetCursor(fn_LoadCursorW(NULL, IDC_WAIT));
	}

	WaitCursor::~WaitCursor()
	{
		fn_SetCursor(m_hOrigCursor);
	}

} // namespace zgui