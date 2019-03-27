#ifndef __ZGUI_TEXT_H_
#define __ZGUI_TEXT_H_

#ifdef ZGUI_USE_TEXT

namespace zgui {

class ITextCallback
{
public:
	virtual String getText(Control* pText) = 0;
};

class Text : public Label
{
public:
	Text();
	~Text();

	const String& getClass() const;
	UINT GetControlFlags() const;
	LPVOID getInterface(const String& name);

	void setTextCallback(ITextCallback* pTextCallback);

	String* GetLinkContent(int iIndex);

	void DoEvent(TEventUI& event);
	SIZE EstimateSize(SIZE szAvailable);
//         void setAttribute(const String& pstrName, const String& pstrValue);

	void PaintText(HDC hDC);

protected:
	enum {
		MAX_LINK = 16
	};
	int _nLinks;
	RECT _rcLinks[MAX_LINK];
	String _sLinks[MAX_LINK];
	int _nHoverLink;
	ITextCallback* _pTextCallback;

private:
	static const String CLASS_NAME;
};

} // namespace zgui

#endif // ZGUI_USE_TEXT

#endif // __ZGUI_TEXT_H_
