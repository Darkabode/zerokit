#ifndef __ZGUI_LIST_H_
#define __ZGUI_LIST_H_

#ifdef ZGUI_USE_LIST

namespace zgui {

typedef int (CALLBACK *PULVCompareFunc)(UINT_PTR, UINT_PTR, UINT_PTR);

class ListHeader;

#define UILIST_MAX_COLUMNS 32

struct ListInfo
{
    ListInfo()
    {
    }

    ~ListInfo()
    {
    }

    int nColumns;
    RECT rcColumn[UILIST_MAX_COLUMNS];
    int nFont;
    UINT uTextStyle;
    RECT rcTextPadding;
    DWORD dwTextColor;
    DWORD dwBkColor;
    String sBkImage;
    bool bAlternateBk;
    DWORD dwSelectedTextColor;
    DWORD dwSelectedBkColor;
    String sSelectedImage;
    DWORD dwHotTextColor;
    DWORD dwHotBkColor;
    String sHotImage;
    DWORD dwDisabledTextColor;
    DWORD dwDisabledBkColor;
    String sDisabledImage;
    DWORD dwLineColor;
    bool bShowHtml;
    bool bMultiExpandable;
	SIZE szCheckImg;
	SIZE szIconImg;
	bool bShowVLine;
	bool bShowHLine;
};


/////////////////////////////////////////////////////////////////////////////////////

class IListCallback
{
public:
	virtual const String&  GetItemText(Control* pList, int iItem, int iSubItem, uint32_t* pColor) = 0;
};

class IListOwner
{
public:
    virtual ListInfo* GetListInfo() = 0;
    virtual int GetCurSel() const = 0;
    virtual bool SelectItem(int iIndex, bool bTakeFocus = false) = 0;
    virtual void DoEvent(TEventUI& event) = 0;
};

class IList : public IListOwner
{
public:
    virtual ListHeader* GetHeader() const = 0;
    virtual Container* GetList() const = 0;
    virtual IListCallback* GetTextCallback() const = 0;
    virtual void SetTextCallback(IListCallback* pCallback) = 0;
    virtual bool ExpandItem(int iIndex, bool bExpand = true) = 0;
    virtual int GetExpandedItem() const = 0;

	virtual bool SelectMultiItem(int iIndex, bool bTakeFocus = false) = 0;
	virtual void SetSingleSelect(bool bSingleSel) = 0;
	virtual bool GetSingleSelect() const = 0;
	virtual bool UnSelectItem(int iIndex) = 0;
	virtual void SelectAllItems() = 0;
	virtual void UnSelectAllItems() = 0;
	virtual int GetSelectItemCount() const = 0;
	virtual int GetNextSelItem(int nItem) const = 0;
};

class IListItem
{
public:
    virtual int GetIndex() const = 0;
    virtual void SetIndex(int iIndex) = 0;
    virtual IListOwner* GetOwner() = 0;
    virtual void SetOwner(Control* pOwner) = 0;
    virtual bool IsSelected() const = 0;
    virtual bool Select(bool bSelect = true) = 0;
    virtual bool IsExpanded() const = 0;
    virtual bool Expand(bool bExpand = true) = 0;
    virtual void DrawItemText(HDC hDC, const RECT& rcItem) = 0;
};


/////////////////////////////////////////////////////////////////////////////////////

class ListBody;
class ListHeader;

class List : public VerticalLayout, public IList
{
public:
    List();

    const String& getClass() const;
    UINT GetControlFlags() const;
    LPVOID getInterface(const String& name);

    bool GetScrollSelect();
    void SetScrollSelect(bool bScrollSelect);
    int GetCurSel() const;
    bool SelectItem(int iIndex, bool bTakeFocus = false);

	bool SelectMultiItem(int iIndex, bool bTakeFocus = false);
	void SetSingleSelect(bool bSingleSel);
	bool GetSingleSelect() const;
	bool UnSelectItem(int iIndex);
	void SelectAllItems();
	void UnSelectAllItems();
	int GetSelectItemCount() const;
	int GetNextSelItem(int nItem) const;

    ListHeader* GetHeader() const;  
    Container* GetList() const;
    ListInfo* GetListInfo();

    Control* getItem(int index) const;
    int indexOf(Control* pControl) const;
	bool setItem(int iIndex, Control* pControl);
    int getCount() const;
    bool add(Control* pControl);
	bool insert(int iIndex, Control* pControl);
    bool remove(Control* pControl);
    bool removeAt(int iIndex);
    void removeAll();

    void EnsureVisible(int iIndex);
    void Scroll(int dx, int dy);

    int GetChildPadding() const;
    void SetChildPadding(int iPadding);

    void SetItemFont(int index);
    void SetItemTextStyle(UINT uStyle);
    void SetItemTextPadding(RECT rc);
    void SetItemTextColor(DWORD dwTextColor);
    void SetItemBkColor(DWORD dwBkColor);
    void SetItemBkImage(const String& pStrImage);
    void SetAlternateBk(bool bAlternateBk);
    void SetSelectedItemTextColor(DWORD dwTextColor);
    void SetSelectedItemBkColor(DWORD dwBkColor);
    void SetSelectedItemImage(const String& pStrImage); 
    void SetHotItemTextColor(DWORD dwTextColor);
    void SetHotItemBkColor(DWORD dwBkColor);
    void SetHotItemImage(const String& pStrImage);
    void SetDisabledItemTextColor(DWORD dwTextColor);
    void SetDisabledItemBkColor(DWORD dwBkColor);
    void SetDisabledItemImage(const String& pStrImage);
    void SetItemLineColor(DWORD dwLineColor);
	void SetCheckImgSize(SIZE szCheckImg);
	void SetIconImgSize(SIZE szIconImg);
	void SetShowVLine(bool bVLine);
	void SetShowHLine(bool bHLine);
    bool IsItemShowHtml();
    void SetItemShowHtml(bool bShowHtml = true);
	RECT GetItemTextPadding() const;
	DWORD GetItemTextColor() const;
	DWORD GetItemBkColor() const;
	const String& GetItemBkImage() const;
    bool IsAlternateBk() const;
	DWORD GetSelectedItemTextColor() const;
	DWORD GetSelectedItemBkColor() const;
	const String& GetSelectedItemImage() const;
	DWORD GetHotItemTextColor() const;
	DWORD GetHotItemBkColor() const;
	const String& GetHotItemImage() const;
	DWORD GetDisabledItemTextColor() const;
	DWORD GetDisabledItemBkColor() const;
	const String& GetDisabledItemImage() const;
	DWORD GetItemLineColor() const;
	SIZE GetCheckImgSize() const;
	SIZE GetIconImgSize() const;
	bool IsShowVLine() const;
	bool IsShowHLine() const;

    void SetMultiExpanding(bool bMultiExpandable); 
    int GetExpandedItem() const;
	void SetMultipleItem(bool bMultipleable);
	bool GetMultipleItem() const;
    bool ExpandItem(int iIndex, bool bExpand = true);

    void SetPos(RECT rc);
    void DoEvent(TEventUI& event);
    void setAttribute(const String& pstrName, const String& pstrValue);

    IListCallback* GetTextCallback() const;
    void SetTextCallback(IListCallback* pCallback);

    SIZE getScrollPos() const;
    SIZE getScrollRange() const;
    void setScrollPos(SIZE szPos);
    void lineUp();
    void lineDown();
    void pageUp();
    void pageDown();
    void homeUp();
    void endDown();
    void lineLeft();
    void lineRight();
    void pageLeft();
    void pageRight();
    void homeLeft();
    void endRight();
    void enableScrollBar(bool bEnableVertical = true, bool bEnableHorizontal = false);
    virtual ScrollBar* getVerticalScrollBar() const;
    virtual ScrollBar* getHorizontalScrollBar() const;

	BOOL SortItems(PULVCompareFunc pfnCompare, UINT_PTR dwData);
protected:
	int GetMinSelItemIndex();
	int GetMaxSelItemIndex();

protected:
    bool m_bScrollSelect;
    //int m_iCurSel;
	bool m_bSingleSel;
	Array<int> m_aSelItems;
    int m_iExpandedItem;
    IListCallback* m_pCallback;
    ListBody* m_pList;
    ListHeader* _pHeader;
    ListInfo m_ListInfo;

private:
	static const String CLASS_NAME;
};

/////////////////////////////////////////////////////////////////////////////////////

class ListBody : public VerticalLayout
{
public:
    ListBody(List* pOwner);

    void setScrollPos(SIZE szPos);
    void SetPos(RECT rc);
    void DoEvent(TEventUI& event);

	BOOL SortItems(PULVCompareFunc pfnCompare, UINT_PTR dwData);

protected:
	static int ItemComareFunc(void *pvlocale, const void *item1, const void *item2);
	int ItemComareFunc(const void *item1, const void *item2);

protected:
    List* _pOwner;
	PULVCompareFunc m_pCompareFunc;
	UINT_PTR m_compareData;
};

/////////////////////////////////////////////////////////////////////////////////////
	
class ListHeader : public HorizontalLayout
{
public:
    ListHeader();

    const String& getClass() const;
    LPVOID getInterface(const String& name);

    SIZE EstimateSize(SIZE szAvailable);

private:
	static const String CLASS_NAME;
};


/////////////////////////////////////////////////////////////////////////////////////

class ListHeaderItem : public Control
{
public:
    ListHeaderItem();

    const String& getClass() const;
    LPVOID getInterface(const String& name);
    UINT GetControlFlags() const;

    void SetEnabled(bool bEnable = true);

	bool IsDragable() const;
    void SetDragable(bool bDragable);
	DWORD GetSepWidth() const;
    void SetSepWidth(int iWidth);
	DWORD GetTextStyle() const;
    void SetTextStyle(UINT uStyle);
	DWORD GetTextColor() const;
    void SetTextColor(DWORD dwTextColor);
	void SetTextPadding(RECT rc);
	RECT GetTextPadding() const;
    void SetFont(int index);
    bool IsShowHtml();
    void SetShowHtml(bool bShowHtml = true);
    const String& GetNormalImage() const;
    void SetNormalImage(const String& pStrImage);
    const String& GetHotImage() const;
    void SetHotImage(const String& pStrImage);
    const String& GetPushedImage() const;
    void SetPushedImage(const String& pStrImage);
    const String& GetFocusedImage() const;
    void SetFocusedImage(const String& pStrImage);
    const String& GetSepImage() const;
    void setSepImage(const String& pStrImage);
	void setSepColor(DWORD dwSepColor);

    void DoEvent(TEventUI& event);
    SIZE EstimateSize(SIZE szAvailable);
    void setAttribute(const String& pstrName, const String& pstrValue);
    RECT GetThumbRect() const;

    void PaintText(HDC hDC);
    void PaintStatusImage(HDC hDC);

protected:
    POINT ptLastMouse;
    bool _bDragable;
    UINT _uButtonState;
    int _iSepWidth;
    DWORD _dwTextColor;
	DWORD _dwSepColor;
    int _iFont;
    UINT _uTextStyle;
    bool _bShowHtml;
	RECT m_rcTextPadding;
    String m_sNormalImage;
    String m_sHotImage;
    String m_sPushedImage;
    String m_sFocusedImage;
    String _sSepImage;
    String m_sSepImageModify;

private:
	static const String CLASS_NAME;
};


/////////////////////////////////////////////////////////////////////////////////////

class ListElement : public Control, public IListItem
{
public:
    ListElement();

    const String& getClass() const;
    UINT GetControlFlags() const;
    LPVOID getInterface(const String& name);

    void SetEnabled(bool bEnable = true);

    int GetIndex() const;
    void SetIndex(int iIndex);

    IListOwner* GetOwner();
    void SetOwner(Control* pOwner);
	void SetVisible(bool bVisible = true);

    bool IsSelected() const;
    bool Select(bool bSelect = true);
    bool IsExpanded() const;
    bool Expand(bool bExpand = true);

    void Invalidate();
    bool Activate();

    void DoEvent(TEventUI& event);
    void setAttribute(const String& pstrName, const String& pstrValue);

    void DrawItemBk(HDC hDC, const RECT& rcItem);

protected:
    int m_iIndex;
    bool m_bSelected;
    UINT m_uButtonState;
    IListOwner* _pOwner;

private:
	static const String CLASS_NAME;
};


/////////////////////////////////////////////////////////////////////////////////////

class ListLabelElement : public ListElement
{
public:
    ListLabelElement();

    const String& getClass() const;
    LPVOID getInterface(const String& name);

    void DoEvent(TEventUI& event);
    SIZE EstimateSize(SIZE szAvailable);
    void DoPaint(HDC hDC, const RECT& rcPaint);

    void DrawItemText(HDC hDC, const RECT& rcItem);

private:
	static const String CLASS_NAME;
};


/////////////////////////////////////////////////////////////////////////////////////

class ListTextElement : public ListLabelElement
{
public:
    ListTextElement();
    ~ListTextElement();

    const String& getClass() const;
    LPVOID getInterface(const String& name);
    UINT GetControlFlags() const;

	bool Select(bool bSelect = true);

    const String& getText(int iIndex) const;
	void setText(int iIndex, const String& pstrText, uint32_t color = 0, int uTextStyle = -1);

    void SetOwner(Control* pOwner);
    String* GetLinkContent(int iIndex);

    void DoEvent(TEventUI& event);
    SIZE EstimateSize(SIZE szAvailable);

    void DrawItemText(HDC hDC, const RECT& rcItem);

protected:
    enum { MAX_LINK = 8 };
    int m_nLinks;
    RECT m_rcLinks[MAX_LINK];
    String m_sLinks[MAX_LINK];
    int m_nHoverLink;
    IList* _pOwner;
    Array<String*> _texts;
	Array<uint32_t> _textColors;
	Array<int> m_uTextsStyle;

private:
	static const String CLASS_NAME;
};

/////////////////////////////////////////////////////////////////////////////////////

class ListContainerElement : public HorizontalLayout, public IListItem
{
public:
    ListContainerElement();

    const String& getClass() const;
    UINT GetControlFlags() const;
    LPVOID getInterface(const String& name);

    int GetIndex() const;
    void SetIndex(int iIndex);

    IListOwner* GetOwner();
    void SetOwner(Control* pOwner);
	void SetVisible(bool bVisible = true);
    void SetEnabled(bool bEnable = true);

    bool IsSelected() const;
    bool Select(bool bSelect = true);
    bool IsExpanded() const;
    bool Expand(bool bExpand = true);

    void Invalidate();
    bool Activate();

    void DoEvent(TEventUI& event);
    void setAttribute(const String& pstrName, const String& pstrValue);
    void DoPaint(HDC hDC, const RECT& rcPaint);

	void SetPos(RECT rc);

    void DrawItemText(HDC hDC, const RECT& rcItem);    
    void DrawItemBk(HDC hDC, const RECT& rcItem);

protected:
    int m_iIndex;
    bool m_bSelected;
    UINT m_uButtonState;
    IListOwner* _pOwner;

private:
	static const String CLASS_NAME;
};

} // namespace zgui

#endif // ZGUI_USE_LIST

#endif // __ZGUI_LIST_H_
