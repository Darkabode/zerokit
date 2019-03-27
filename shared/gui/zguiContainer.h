#ifndef __ZGUI_CONTAINER_H_
#define __ZGUI_CONTAINER_H_

#ifdef ZGUI_USE_SCROLLBAR

#ifdef ZGUI_USE_CONTAINER

namespace zgui {

class IContainer
{
public:
    virtual Control* getItem(int index) const = 0;
    virtual int indexOf(Control* pControl) const  = 0;
	virtual bool setItem(int iIndex, Control* pControl) = 0;
    virtual int getCount() const = 0;
    virtual bool add(Control* pControl) = 0;
	virtual bool insert(int iIndex, Control* pControl) = 0;
    virtual bool remove(Control* pControl) = 0;
    virtual bool removeAt(int iIndex)  = 0;
    virtual void removeAll() = 0;
};


class ScrollBar;

class Container : public Control, public IContainer
{
public:
    Container();
    virtual ~Container();

    const String& getClass() const;
    LPVOID getInterface(const String& name);

    Control* getItem(int index) const;
    int indexOf(Control* pControl) const;
	bool setItem(int iIndex, Control* pControl);
    int getCount() const;
    bool add(Control* pControl);
	bool insert(int iIndex, Control* pControl);
    bool remove(Control* pControl);
    bool removeAt(int iIndex);
    void removeAll();

    void DoEvent(TEventUI& event);
    virtual void SetVisible(bool bVisible = true);
	virtual void SetEnabled(bool bEnable = true);
	virtual void SetInternVisible(bool bVisible = true);
    void SetMouseEnabled(bool bEnable = true);

	virtual void updateText();

    virtual RECT getInset() const;
    virtual void setInset(RECT& rcInset);
    virtual int getChildPadding() const;
    virtual void setChildPadding(int iPadding);
    virtual bool IsAutoDestroy() const;
    virtual void SetAutoDestroy(bool bAuto);
    virtual bool IsDelayedDestroy() const;
    virtual void SetDelayedDestroy(bool bDelayed);
    virtual bool IsMouseChildEnabled() const;
    virtual void SetMouseChildEnabled(bool bEnable = true);

    virtual int FindSelectable(int iIndex, bool bForward = true) const;

    void SetPos(RECT rc);
    void DoPaint(HDC hDC, const RECT& rcPaint);

    void setAttribute(const String& pstrName, const String& pstrValue);

    void SetManager(PaintManager* pManager, Control* pParent, bool bInit = true);
    Control* FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags);

	bool SetSubControlText(LPCTSTR pstrSubControlName,LPCTSTR pstrText);
	bool SetSubControlFixedHeight(LPCTSTR pstrSubControlName,int cy);
	bool SetSubControlFixedWdith(LPCTSTR pstrSubControlName,int cx);
	bool SetSubControlUserData(LPCTSTR pstrSubControlName,LPCTSTR pstrText);

	String GetSubControlText(LPCTSTR pstrSubControlName);
	int GetSubControlFixedHeight(LPCTSTR pstrSubControlName);
	int GetSubControlFixedWdith(LPCTSTR pstrSubControlName);
	String GetSubControlUserData(LPCTSTR pstrSubControlName);
	Control* FindSubControl(LPCTSTR pstrSubControlName);

    virtual SIZE getScrollPos() const;
    virtual SIZE getScrollRange() const;
    virtual void setScrollPos(SIZE szPos);
    virtual void lineUp();
    virtual void lineDown();
    virtual void pageUp();
    virtual void pageDown();
    virtual void homeUp();
    virtual void endDown();
    virtual void lineLeft();
    virtual void lineRight();
    virtual void pageLeft();
    virtual void pageRight();
    virtual void homeLeft();
    virtual void endRight();
    virtual void enableScrollBar(bool bEnableVertical = true, bool bEnableHorizontal = false);
    virtual ScrollBar* getVerticalScrollBar() const;
    virtual ScrollBar* getHorizontalScrollBar() const;

protected:
    virtual void SetFloatPos(int iIndex);
    virtual void processScrollBar(RECT rc, int cxRequired, int cyRequired);

protected:
	Array<Control*> _items;
    RECT _rcInset;
    int m_iChildPadding;
    bool m_bAutoDestroy;
    bool m_bDelayedDestroy;
    bool m_bMouseChildEnabled;
    bool m_bScrollProcess;
	bool _bNoInsetVScroll;
	bool _bNoInsetHScroll;

    ScrollBar* _pVerticalScrollBar;
    ScrollBar* _pHorizontalScrollBar;

private:
	static const String CLASS_NAME;
};

} // namespace zgui

#endif // ZGUI_USE_CONTAINER

#endif // ZGUI_USE_SCROLLBAR

#endif // __ZGUI_CONTAINER_H_
