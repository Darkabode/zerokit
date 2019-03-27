#ifndef __ZGUI_COMBO_H_
#define __ZGUI_COMBO_H_

#ifdef ZGUI_USE_COMBO

namespace zgui {

class CComboWnd;

class Combo : public Container, public IListOwner
{
    friend class CComboWnd;
public:
    Combo();

    const String& getClass() const;
    LPVOID getInterface(const String& name);

    void DoInit();
    UINT GetControlFlags() const;

    String getText() const;
    void SetEnabled(bool bEnable = true);

    String GetDropBoxAttributeList();
	void SetDropBoxAttributeList(const String& pstrList);
    SIZE GetDropBoxSize() const;
    void SetDropBoxSize(SIZE szDropBox);

    int GetCurSel() const;  
    bool SelectItem(int iIndex, bool bTakeFocus = false);

	bool setItem(int iIndex, Control* pControl);
    bool add(Control* pControl);
	bool insert(int iIndex, Control* pControl);
    bool remove(Control* pControl);
    bool removeAt(int iIndex);
    void removeAll();

    bool Activate();

    RECT GetTextPadding() const;
    void SetTextPadding(RECT rc);
    const String& GetNormalImage() const;
	void SetNormalImage(const String& pStrImage);
	const String& GetHotImage() const;
	void SetHotImage(const String& pStrImage);
	const String& GetPushedImage() const;
	void SetPushedImage(const String& pStrImage);
	const String& GetFocusedImage() const;
	void SetFocusedImage(const String& pStrImage);
	const String& GetDisabledImage() const;
	void SetDisabledImage(const String& pStrImage);

    ListInfo* GetListInfo();
    void SetItemFont(int index);
    void SetItemTextStyle(UINT uStyle);
	RECT GetItemTextPadding() const;
    void SetItemTextPadding(RECT rc);
	DWORD GetItemTextColor() const;
    void SetItemTextColor(DWORD dwTextColor);
	DWORD GetItemBkColor() const;
    void SetItemBkColor(DWORD dwBkColor);
	const String& GetItemBkImage() const;
	void SetItemBkImage(const String& pStrImage);
    bool IsAlternateBk() const;
    void SetAlternateBk(bool bAlternateBk);
	DWORD GetSelectedItemTextColor() const;
    void SetSelectedItemTextColor(DWORD dwTextColor);
	DWORD GetSelectedItemBkColor() const;
    void SetSelectedItemBkColor(DWORD dwBkColor);
	const String& GetSelectedItemImage() const;
	void SetSelectedItemImage(const String& pStrImage);
	DWORD GetHotItemTextColor() const;
    void SetHotItemTextColor(DWORD dwTextColor);
	DWORD GetHotItemBkColor() const;
    void SetHotItemBkColor(DWORD dwBkColor);
	const String& GetHotItemImage() const;
	void SetHotItemImage(const String& pStrImage);
	DWORD GetDisabledItemTextColor() const;
    void SetDisabledItemTextColor(DWORD dwTextColor);
	DWORD GetDisabledItemBkColor() const;
    void SetDisabledItemBkColor(DWORD dwBkColor);
	const String& GetDisabledItemImage() const;
	void SetDisabledItemImage(const String& pStrImage);
	DWORD GetItemLineColor() const;
    void SetItemLineColor(DWORD dwLineColor);
    bool IsItemShowHtml();
    void SetItemShowHtml(bool bShowHtml = true);

    SIZE EstimateSize(SIZE szAvailable);
    void SetPos(RECT rc);
    void DoEvent(TEventUI& event);
    void setAttribute(const String& pstrName, const String& pstrValue);
    
    void DoPaint(HDC hDC, const RECT& rcPaint);
    void PaintText(HDC hDC);
    void PaintStatusImage(HDC hDC);

protected:
    CComboWnd* m_pWindow;

    int m_iCurSel;
    RECT m_rcTextPadding;
    String m_sDropBoxAttributes;
    SIZE m_szDropBox;
    UINT m_uButtonState;

    String m_sNormalImage;
    String m_sHotImage;
    String m_sPushedImage;
    String m_sFocusedImage;
    String m_sDisabledImage;

    ListInfo m_ListInfo;

private:
	static const String CLASS_NAME;
};

} // namespace zgui

#endif // ZGUI_USE_COMBO

#endif // __ZGUI_COMBO_H_
