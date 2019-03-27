#ifndef __ZGUI_TREEVIEW_H_
#define __ZGUI_TREEVIEW_H_

#if defined(ZGUI_USE_LIST) && defined(ZGUI_USE_TREEVIEW)

namespace zgui {
	
class TreeView;
class CheckBox;
class Label;

class TreeNode : public ListContainerElement
{
public:
	TreeNode(TreeNode* _ParentNode = NULL);
	~TreeNode(void);

	const String& getClass() const;
	LPVOID	getInterface(const String& pstrName);
	void	DoEvent(TEventUI& event);
	void	Invalidate();
	bool	Select(bool bSelect = true);

	bool	Add(Control* _pTreeNodeUI);
	bool	AddAt(Control* pControl, int iIndex);
	bool	Remove(Control* pControl);

	void	SetVisibleTag(bool _IsVisible);
	bool	GetVisibleTag();
	void	SetItemText(const String& pstrValue);
	String	GetItemText();
	void	CheckBoxSelected(bool _Selected);
	bool	IsCheckBoxSelected() const;
	bool	IsHasChild() const;
	long	GetTreeLevel() const;
	bool	AddChildNode(TreeNode* _pTreeNodeUI);
	bool	RemoveAt(TreeNode* _pTreeNodeUI);
	void	SetParentNode(TreeNode* _pParentTreeNode);
	TreeNode* GetParentNode();
	long	GetCountChild();
	void	SetTreeView(TreeView* _CTreeViewUI);
	TreeView* GetTreeView();
	TreeNode* GetChildNode(int _nIndex);
	void	SetVisibleFolderBtn(bool _IsVisibled);
	bool	GetVisibleFolderBtn();
	void	SetVisibleCheckBtn(bool _IsVisibled);
	bool	GetVisibleCheckBtn();
	void	SetItemTextColor(DWORD _dwItemTextColor);
	DWORD	GetItemTextColor() const;
	void	SetItemHotTextColor(DWORD _dwItemHotTextColor);
	DWORD	GetItemHotTextColor() const;
	void	SetSelItemTextColor(DWORD _dwSelItemTextColor);
	DWORD	GetSelItemTextColor() const;
	void	SetSelItemHotTextColor(DWORD _dwSelHotItemTextColor);
	DWORD	GetSelItemHotTextColor() const;

	void setAttribute(const String& pstrName, const String& pstrValue);

	Array<void*> GetTreeNodes();

	int GetTreeIndex();
	int GetNodeIndex();

	HorizontalLayout* GetTreeNodeHoriznotal() const { return pHoriz; };
	Button* GetFolderButton() const { return pFolderButton; };
	Label* GetDottedLine() const { return pDottedLine; };
	Button* GetCheckBox() const { return pCheckBox; };
	Button* GetItemButton() const { return pItemButton; };

private:
	TreeNode* GetLastNode();
	TreeNode* CalLocation(TreeNode* _pTreeNodeUI);

private:
	long	m_iTreeLavel;
	bool	m_bIsVisable;
	bool	m_bIsCheckBox;
	DWORD	m_dwItemTextColor;
	DWORD	m_dwItemHotTextColor;
	DWORD	m_dwSelItemTextColor;
	DWORD	m_dwSelItemHotTextColor;

	TreeView*			pTreeView;
	HorizontalLayout*	pHoriz;
	Button*			pFolderButton;
	Label*				pDottedLine;
	Button*			pCheckBox;
	Button*				pItemButton;

	TreeNode*			pParentTreeNode;

	Array<void*> _treeNodes;

	static const String CLASS_NAME;
};

class TreeView : public List, public INotifyUI
{
public:
	TreeView(void);
	~TreeView(void);

public:
	virtual const String& getClass() const;
	virtual LPVOID getInterface(const String& pstrName);
	virtual bool add(TreeNode* pControl );
	virtual long insert(int iIndex, TreeNode* pControl);
	virtual bool AddAt(TreeNode* pControl,TreeNode* _IndexNode);
	virtual bool remove(TreeNode* pControl);
	virtual bool removeAt(int iIndex);
	virtual void removeAll();
	virtual bool OnCheckBoxChanged(void* param);
	virtual bool OnFolderChanged(void* param);
	virtual bool OnDBClickItem(void* param);
	virtual bool SetItemCheckBox(bool _Selected,TreeNode* _TreeNode = NULL);
	virtual void SetItemExpand(bool _Expanded,TreeNode* _TreeNode = NULL);
	virtual void Notify(TNotifyUI& msg);
	virtual void SetVisibleFolderBtn(bool _IsVisibled);
	virtual bool GetVisibleFolderBtn();
	virtual void SetVisibleCheckBtn(bool _IsVisibled);
	virtual bool GetVisibleCheckBtn();
	virtual void SetItemMinWidth(UINT _ItemMinWidth);
	virtual UINT GetItemMinWidth();
	virtual void SetItemTextColor(DWORD _dwItemTextColor);
	virtual void SetItemHotTextColor(DWORD _dwItemHotTextColor);
	virtual void SetSelItemTextColor(DWORD _dwSelItemTextColor);
	virtual void SetSelItemHotTextColor(DWORD _dwSelHotItemTextColor);
		
	virtual void setAttribute(const String& pstrName, const String& pstrValue);
private:
	UINT m_uItemMinWidth;
	bool m_bVisibleFolderBtn;
	bool m_bVisibleCheckBtn;

	static const String CLASS_NAME;
};

}

#endif // ZGUI_USE_LIST && ZGUI_USE_TREEVIEW

#endif // __ZGUI_TREEVIEW_H_
