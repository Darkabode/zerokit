#include "zgui.h"

#if defined(ZGUI_USE_LIST) && defined(ZGUI_USE_TREEVIEW)

#pragma warning( disable: 4251 )
namespace zgui {

const String TreeNode::CLASS_NAME = ZGUI_TREENODE;

TreeNode::TreeNode( TreeNode* _ParentNode /*= NULL*/ )
{
	m_dwItemTextColor		= 0x00000000;
	m_dwItemHotTextColor	= 0;
	m_dwSelItemTextColor	= 0;
	m_dwSelItemHotTextColor	= 0;

	pTreeView		= NULL;
	m_bIsVisable	= true;
	m_bIsCheckBox	= false;
	pParentTreeNode	= NULL;

	pHoriz = new HorizontalLayout();
	pFolderButton = new Button();
	pDottedLine = new Label();
	pCheckBox = new Button();
	pItemButton = new Button();

	this->SetFixedHeight(18);
	this->SetFixedWidth(250);
	pFolderButton->SetFixedWidth(GetFixedHeight());
	pDottedLine->SetFixedWidth(2);
	pCheckBox->SetFixedWidth(GetFixedHeight());
	pItemButton->setAttribute("align", "left");

	pDottedLine->SetVisible(false);
	pCheckBox->SetVisible(false);
	pItemButton->SetMouseEnabled(false);

	if (_ParentNode) {
		if (_ParentNode->getClass().compareIgnoreCase(ZGUI_TREENODE) != 0) {
			return;
		}

		pDottedLine->SetVisible(_ParentNode->IsVisible());
		pDottedLine->SetFixedWidth(_ParentNode->GetDottedLine()->GetFixedWidth()+16);
		this->SetParentNode(_ParentNode);
	}

	pHoriz->add(pDottedLine);
	pHoriz->add(pFolderButton);
	pHoriz->add(pCheckBox);
	pHoriz->add(pItemButton);
	Add(pHoriz);
}
	
TreeNode::~TreeNode( void )
{

}

const String& TreeNode::getClass() const
{
	return CLASS_NAME;
}

LPVOID TreeNode::getInterface(const String& pstrName)
{
	if (pstrName == ZGUI_TREENODE) {
		return static_cast<TreeNode*>(this);
	}
	return ListContainerElement::getInterface(pstrName);
}
	
void TreeNode::DoEvent( TEventUI& event )
{
	if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
		if (_pOwner != 0) {
			_pOwner->DoEvent(event);
		}
		else {
			Container::DoEvent(event);
		}
		return;
	}

	ListContainerElement::DoEvent(event);

	if (event.Type == UIEVENT_DBLCLICK) {
		if (IsEnabled()) {
			_pManager->SendNotify(this, "itemdbclick");
			Invalidate();
		}
		return;
	}
	if (event.Type == UIEVENT_MOUSEENTER) {
		if( IsEnabled()) {
			if (m_bSelected && GetSelItemHotTextColor()) {
				pItemButton->SetTextColor(GetSelItemHotTextColor());
			}
			else {
				pItemButton->SetTextColor(GetItemHotTextColor());
			}
		}
		else {
			pItemButton->SetTextColor(pItemButton->GetDisabledTextColor());
		}

		return;
	}
	if (event.Type == UIEVENT_MOUSELEAVE) {
		if (IsEnabled()) {
			if (m_bSelected && GetSelItemTextColor()) {
				pItemButton->SetTextColor(GetSelItemTextColor());
			}
			else if (!m_bSelected) {
				pItemButton->SetTextColor(GetItemTextColor());
			}
		}
		else {
			pItemButton->SetTextColor(pItemButton->GetDisabledTextColor());
		}

		return;
	}
}

void TreeNode::Invalidate()
{
	if (!IsVisible()) {
		return;
	}

	if (GetParent()) {
		Container* pParentContainer = static_cast<Container*>(GetParent()->getInterface("Container"));
		if (pParentContainer) {
			RECT rc = pParentContainer->GetPos();
			RECT rcInset = pParentContainer->getInset();
			rc.left += rcInset.left;
			rc.top += rcInset.top;
			rc.right -= rcInset.right;
			rc.bottom -= rcInset.bottom;
			ScrollBar* pVerticalScrollBar = pParentContainer->getVerticalScrollBar();
			if (pVerticalScrollBar && pVerticalScrollBar->IsVisible()) {
				rc.right -= pVerticalScrollBar->GetFixedWidth();
			}
			ScrollBar* pHorizontalScrollBar = pParentContainer->getHorizontalScrollBar();
			if (pHorizontalScrollBar && pHorizontalScrollBar->IsVisible()) {
				rc.bottom -= pHorizontalScrollBar->GetFixedHeight();
			}

			RECT invalidateRc = _rcItem;
			if( !fn_IntersectRect(&invalidateRc, &_rcItem, &rc) ) 
				return;

			Control* pParent = GetParent();
			RECT rcTemp;
			RECT rcParent;
			while (pParent = pParent->GetParent()) {
				rcTemp = invalidateRc;
				rcParent = pParent->GetPos();
				if (!fn_IntersectRect(&invalidateRc, &rcTemp, &rcParent)) {
					return;
				}
			}

			if (_pManager != 0) {
				_pManager->Invalidate(invalidateRc);
			}
		}
		else {
			Container::Invalidate();
		}
	}
	else {
		Container::Invalidate();
	}
}
	
bool TreeNode::Select( bool bSelect /*= true*/ )
{
	bool nRet = ListContainerElement::Select(bSelect);
	if (m_bSelected) {
		pItemButton->SetTextColor(GetSelItemTextColor());
	}
	else {
		pItemButton->SetTextColor(GetItemTextColor());
	}

	return nRet;
}

bool TreeNode::Add(Control* _pTreeNodeUI )
{
	if (_pTreeNodeUI->getClass().compareIgnoreCase(ZGUI_TREENODE) == 0) {
		return AddChildNode((TreeNode*)_pTreeNodeUI);
	}

	return ListContainerElement::add(_pTreeNodeUI);
}

bool TreeNode::AddAt( Control* pControl, int iIndex)
{
	if (static_cast<TreeNode*>(pControl->getInterface(ZGUI_TREENODE)) == NULL) {
		return false;
	}

	TreeNode* pIndexNode = static_cast<TreeNode*>(_treeNodes[iIndex]);
	if (pIndexNode == 0){
		_treeNodes.add(pControl);
	}
	else if (pIndexNode) {
		_treeNodes.insert(iIndex, pControl);
	}

	if (!pIndexNode && pTreeView && pTreeView->getItem(GetTreeIndex() + 1)) {
		pIndexNode = static_cast<TreeNode*>(pTreeView->getItem(GetTreeIndex() + 1)->getInterface(ZGUI_TREENODE));
	}

	pControl = CalLocation((TreeNode*)pControl);

	if (pTreeView && pIndexNode) {
		return pTreeView->AddAt((TreeNode*)pControl, pIndexNode);
	}
	else {
		return pTreeView->add((TreeNode*)pControl);
	}

	return true;
}

bool TreeNode::Remove( Control* pControl )
{
	return RemoveAt((TreeNode*)pControl);
}

void TreeNode::SetVisibleTag( bool _IsVisible )
{
	m_bIsVisable = _IsVisible;
}

bool TreeNode::GetVisibleTag()
{
	return m_bIsVisable;
}

void TreeNode::SetItemText(const String& pstrValue)
{
	pItemButton->setText(pstrValue);
}

String TreeNode::GetItemText()
{
	return pItemButton->getText();
}

void TreeNode::CheckBoxSelected( bool _Selected )
{
	pCheckBox->setSelected(_Selected);
}

bool TreeNode::IsCheckBoxSelected() const
{
	return pCheckBox->isSelected();
}

bool TreeNode::IsHasChild() const
{
	return (_treeNodes.size() != 0);
}
	
bool TreeNode::AddChildNode( TreeNode* _pTreeNodeUI )
{
	if (!_pTreeNodeUI)
		return false;

	if (_pTreeNodeUI->getClass().compareIgnoreCase(ZGUI_TREENODE) != 0) {
		return false;
	}

	_pTreeNodeUI = CalLocation(_pTreeNodeUI);

	bool nRet = true;

	if (pTreeView) {
		TreeNode* pNode = static_cast<TreeNode*>(_treeNodes.getLast());
		if (!pNode || !pNode->GetLastNode()) {
			nRet = pTreeView->insert(GetTreeIndex() + 1, _pTreeNodeUI) >= 0;
		}
		else {
			nRet = pTreeView->insert(pNode->GetLastNode()->GetTreeIndex() + 1, _pTreeNodeUI) >= 0;
		}
	}

	if (nRet) {
		_treeNodes.add(_pTreeNodeUI);
	}

	return nRet;
}

bool TreeNode::RemoveAt( TreeNode* _pTreeNodeUI )
{
	int nIndex = _treeNodes.indexOf(_pTreeNodeUI);
	TreeNode* pNode = static_cast<TreeNode*>(_treeNodes[nIndex]);
	if(pNode && pNode == _pTreeNodeUI) {
		while (pNode->IsHasChild()) {
			pNode->RemoveAt(static_cast<TreeNode*>(pNode->_treeNodes[0]));
		}

		_treeNodes.remove(nIndex);

		if (pTreeView) {
			pTreeView->remove(_pTreeNodeUI);
		}

		return true;
	}
	return false;
}

void TreeNode::SetParentNode( TreeNode* _pParentTreeNode )
{
	pParentTreeNode = _pParentTreeNode;
}

TreeNode* TreeNode::GetParentNode()
{
	return pParentTreeNode;
}

long TreeNode::GetCountChild()
{
	return _treeNodes.size();
}

void TreeNode::SetTreeView( TreeView* _CTreeViewUI )
{
	pTreeView = _CTreeViewUI;
}

TreeView* TreeNode::GetTreeView()
{
	return pTreeView;
}

void TreeNode::setAttribute(const String& pstrName, const String& pstrValue)
{
	if (pstrName == "text") {
		pItemButton->setText(pstrValue);
	}
	else if (pstrName == "horizattr") {
		pHoriz->applyAttributeList(pstrValue);
	}
	else if (pstrName == "dotlineattr") {
		pDottedLine->applyAttributeList(pstrValue);
	}
	else if (pstrName == "folderattr") {
		pFolderButton->applyAttributeList(pstrValue);
	}
	else if (pstrName == "checkboxattr") {
		pCheckBox->applyAttributeList(pstrValue);
	}
	else if (pstrName == "itemattr") {
		pItemButton->applyAttributeList(pstrValue);
	}
	else if (pstrName == "itemtextcolor") {
		SetItemTextColor((uint32_t)pstrValue.getHexValue32());
	}
	else if (pstrName == "itemhottextcolor") {
		SetItemHotTextColor((uint32_t)pstrValue.getHexValue32());
	}
	else if (pstrName == "selitemtextcolor") {
		SetSelItemTextColor((uint32_t)pstrValue.getHexValue32());
	}
	else if (pstrName == "selitemhottextcolor") {
		SetSelItemHotTextColor((uint32_t)pstrValue.getHexValue32());
	}
	else {
		ListContainerElement::setAttribute(pstrName, pstrValue);
	}
}

Array<void*> TreeNode::GetTreeNodes()
{
	return _treeNodes;
}

TreeNode* TreeNode::GetChildNode( int _nIndex )
{
	return static_cast<TreeNode*>(_treeNodes[_nIndex]);
}

void TreeNode::SetVisibleFolderBtn( bool _IsVisibled )
{
	pFolderButton->SetVisible(_IsVisibled);
}

bool TreeNode::GetVisibleFolderBtn()
{
	return pFolderButton->IsVisible();
}

void TreeNode::SetVisibleCheckBtn( bool _IsVisibled )
{
	pCheckBox->SetVisible(_IsVisibled);
}

bool TreeNode::GetVisibleCheckBtn()
{
	return pCheckBox->IsVisible();
}
	
int TreeNode::GetTreeIndex()
{
	if (!pTreeView) {
		return -1;
	}

	for (int nIndex = 0; nIndex < pTreeView->getCount(); ++nIndex) {
		if (this == pTreeView->getItem(nIndex)) {
			return nIndex;
		}
	}

	return -1;
}
	
int TreeNode::GetNodeIndex()
{
	if(!GetParentNode() && !pTreeView)
		return -1;

	if(!GetParentNode() && pTreeView)
		return GetTreeIndex();

	return GetParentNode()->GetTreeNodes().indexOf(this);
}

TreeNode* TreeNode::GetLastNode( )
{
	if(!IsHasChild())
		return this;

	TreeNode* nRetNode = NULL;

	for (int nIndex = 0;nIndex < GetTreeNodes().size(); ++nIndex) {
		TreeNode* pNode = static_cast<TreeNode*>(GetTreeNodes().getUnchecked(nIndex));
		if (pNode == 0) {
			continue;
		}

		String aa = pNode->GetItemText(); // ???

		if (pNode->IsHasChild()) {
			nRetNode = pNode->GetLastNode();
		}
		else {
			nRetNode = pNode;
		}
	}
		
	return nRetNode;
}
	
TreeNode* TreeNode::CalLocation( TreeNode* _pTreeNodeUI )
{
	_pTreeNodeUI->GetDottedLine()->SetVisible(true);
	_pTreeNodeUI->GetDottedLine()->SetFixedWidth(pDottedLine->GetFixedWidth()+16);
	_pTreeNodeUI->SetParentNode(this);
	_pTreeNodeUI->GetItemButton()->SetGroup(pItemButton->GetGroup());
	_pTreeNodeUI->SetTreeView(pTreeView);

	return _pTreeNodeUI;
}

void TreeNode::SetItemTextColor( DWORD _dwItemTextColor )
{
	m_dwItemTextColor = _dwItemTextColor;
	pItemButton->SetTextColor(m_dwItemTextColor);
}

DWORD TreeNode::GetItemTextColor() const
{
	return m_dwItemTextColor;
}

void TreeNode::SetItemHotTextColor( DWORD _dwItemHotTextColor )
{
	m_dwItemHotTextColor = _dwItemHotTextColor;
	Invalidate();
}

DWORD TreeNode::GetItemHotTextColor() const
{
	return m_dwItemHotTextColor;
}

void TreeNode::SetSelItemTextColor( DWORD _dwSelItemTextColor )
{
	m_dwSelItemTextColor = _dwSelItemTextColor;
	Invalidate();
}

DWORD TreeNode::GetSelItemTextColor() const
{
	return m_dwSelItemTextColor;
}

void TreeNode::SetSelItemHotTextColor( DWORD _dwSelHotItemTextColor )
{
	m_dwSelItemHotTextColor = _dwSelHotItemTextColor;
	Invalidate();
}

DWORD TreeNode::GetSelItemHotTextColor() const
{
	return m_dwSelItemHotTextColor;
}

/*****************************************************************************/
const String TreeView::CLASS_NAME = ZGUI_TREEVIEW;

TreeView::TreeView( void ) : m_bVisibleFolderBtn(true),m_bVisibleCheckBtn(false),m_uItemMinWidth(0)
{
	this->GetHeader()->SetVisible(false);
}
	
TreeView::~TreeView( void )
{
		
}

const String& TreeView::getClass() const
{
	return CLASS_NAME;
}

LPVOID TreeView::getInterface(const String& pstrName)
{
	if (pstrName == ZGUI_TREEVIEW) {
		return static_cast<TreeView*>(this);
	}
	return List::getInterface(pstrName);
}

bool TreeView::add(TreeNode* pControl)
{
	if (!pControl)
		return false;

	if (pControl->getClass().compareIgnoreCase(ZGUI_TREENODE) != 0) {
		return false;
	}

	pControl->OnNotify += MakeDelegate(this,&TreeView::OnDBClickItem);
	pControl->GetFolderButton()->OnNotify += MakeDelegate(this,&TreeView::OnFolderChanged);
	pControl->GetCheckBox()->OnNotify += MakeDelegate(this,&TreeView::OnCheckBoxChanged);

	pControl->SetVisibleFolderBtn(m_bVisibleFolderBtn);
	pControl->SetVisibleCheckBtn(m_bVisibleCheckBtn);
	if (m_uItemMinWidth > 0) {
		pControl->SetMinWidth(m_uItemMinWidth);
	}

	List::add(pControl);

	if (pControl->GetCountChild() > 0) {
		int nCount = pControl->GetCountChild();
		for (int nIndex = 0;nIndex < nCount; ++nIndex) {
			TreeNode* pNode = pControl->GetChildNode(nIndex);
			if (pNode) {
				add(pNode);
			}
		}
	}

	pControl->SetTreeView(this);
	return true;
}

long TreeView::insert(int iIndex, TreeNode* pControl)
{
	if (!pControl)
		return -1;

	if (pControl->getClass().compareIgnoreCase(ZGUI_TREENODE) != 0) {
		return -1;
	}

	TreeNode* pParent = static_cast<TreeNode*>(getItem(iIndex));
	if (pParent == 0) {
		return -1;
	}

	pControl->OnNotify += MakeDelegate(this,&TreeView::OnDBClickItem);
	pControl->GetFolderButton()->OnNotify += MakeDelegate(this,&TreeView::OnFolderChanged);
	pControl->GetCheckBox()->OnNotify += MakeDelegate(this,&TreeView::OnCheckBoxChanged);

	pControl->SetVisibleFolderBtn(m_bVisibleFolderBtn);
	pControl->SetVisibleCheckBtn(m_bVisibleCheckBtn);

	if (m_uItemMinWidth > 0) {
		pControl->SetMinWidth(m_uItemMinWidth);
	}

	List::insert(iIndex, pControl);

	if (pControl->GetCountChild() > 0) {
		int nCount = pControl->GetCountChild();
		for (int nIndex = 0;nIndex < nCount; ++nIndex) {
			TreeNode* pNode = pControl->GetChildNode(nIndex);
			if (pNode) {
				return insert(iIndex + 1, pNode);
			}
		}
	}
	else {
		return iIndex + 1;
	}

	return -1;
}

bool TreeView::AddAt( TreeNode* pControl,TreeNode* _IndexNode )
{
	if (!_IndexNode && !pControl) {
		return false;
	}

	int nItemIndex = -1;

	for (int nIndex = 0;nIndex < getCount(); ++nIndex) {
		if (_IndexNode == getItem(nIndex)) {
			nItemIndex = nIndex;
			break;
		}
	}

	if (nItemIndex == -1) {
		return false;
	}

	return insert(nItemIndex, pControl) >= 0;
}

bool TreeView::remove(TreeNode* pControl)
{
	if (pControl->GetCountChild() > 0) {
		int nCount = pControl->GetCountChild();
		for (int nIndex = 0;nIndex < nCount; ++nIndex) {
			TreeNode* pNode = pControl->GetChildNode(nIndex);
			if( pNode != 0) {
				pControl->Remove(pNode);
			}
		}
	}
	List::remove(pControl);
	return true;
}

bool TreeView::removeAt( int iIndex )
{
	TreeNode* pItem = (TreeNode*)getItem(iIndex);
	if (pItem->GetCountChild()) {
		remove(pItem);
	}
	return true;
}

void TreeView::removeAll()
{
	List::removeAll();
}

void TreeView::Notify( TNotifyUI& msg )
{
		
}
	
bool TreeView::OnCheckBoxChanged( void* param )
{
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if(pMsg->sType == "selectchanged") {
		Button* pCheckBox = (Button*)pMsg->pSender;
		TreeNode* pItem = (TreeNode*)pCheckBox->GetParent()->GetParent();
		SetItemCheckBox(pCheckBox->isSelected(),pItem);
		return true;
	}
	return true;
}
	
bool TreeView::OnFolderChanged( void* param )
{
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if (pMsg->sType == "selectchanged") {
		Button* pFolder = (Button*)pMsg->pSender;
		TreeNode* pItem = (TreeNode*)pFolder->GetParent()->GetParent();
		pItem->SetVisibleTag(!pFolder->isSelected());
		SetItemExpand(!pFolder->isSelected(),pItem);
		return true;
	}
	return true;
}
	
bool TreeView::OnDBClickItem( void* param )
{
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if (pMsg->sType == "itemdbclick") {
		TreeNode* pItem = static_cast<TreeNode*>(pMsg->pSender);
		Button* pFolder = pItem->GetFolderButton();
		pFolder->setSelected(!pFolder->isSelected());
		pItem->SetVisibleTag(!pFolder->isSelected());
		SetItemExpand(!pFolder->isSelected(),pItem);
		return true;
	}
	return false;
}

bool TreeView::SetItemCheckBox( bool _Selected,TreeNode* _TreeNode /*= NULL*/ )
{
	if(_TreeNode) {
		if (_TreeNode->GetCountChild() > 0) {
			int nCount = _TreeNode->GetCountChild();
			for (int nIndex = 0;nIndex < nCount; ++nIndex) {
				TreeNode* pItem = _TreeNode->GetChildNode(nIndex);
				pItem->GetCheckBox()->setSelected(_Selected);
				if (pItem->GetCountChild()) {
					SetItemCheckBox(_Selected, pItem);
				}
			}
		}
		return true;
	}
	else {
		int nIndex = 0;
		int nCount = getCount();
		while(nIndex < nCount) {
			TreeNode* pItem = (TreeNode*)getItem(nIndex);
			pItem->GetCheckBox()->setSelected(_Selected);
			if(pItem->GetCountChild())
				SetItemCheckBox(_Selected,pItem);

			nIndex++;
		}
		return true;
	}
	return false;
}

void TreeView::SetItemExpand( bool _Expanded,TreeNode* _TreeNode /*= NULL*/ )
{
	if(_TreeNode)
	{
		if(_TreeNode->GetCountChild() > 0)
		{
			int nCount = _TreeNode->GetCountChild();
			for(int nIndex = 0;nIndex < nCount;nIndex++)
			{
				TreeNode* pItem = _TreeNode->GetChildNode(nIndex);
				pItem->SetVisible(_Expanded);

				if(pItem->GetCountChild() && !pItem->GetFolderButton()->isSelected())
					SetItemExpand(_Expanded,pItem);
			}
		}
	}
	else
	{
		int nIndex = 0;
		int nCount = getCount();
		while(nIndex < nCount) {
			TreeNode* pItem = (TreeNode*)getItem(nIndex);

			pItem->SetVisible(_Expanded);

			if (pItem->GetCountChild() && !pItem->GetFolderButton()->isSelected()) {
				SetItemExpand(_Expanded, pItem);
			}

			nIndex++;
		}
	}
}

void TreeView::SetVisibleFolderBtn( bool _IsVisibled )
{
	m_bVisibleFolderBtn = _IsVisibled;
	int nCount = this->getCount();
	for (int nIndex = 0;nIndex < nCount; ++nIndex) {
		TreeNode* pItem = static_cast<TreeNode*>(this->getItem(nIndex));
		pItem->GetFolderButton()->SetVisible(m_bVisibleFolderBtn);
	}
}

bool TreeView::GetVisibleFolderBtn()
{
	return m_bVisibleFolderBtn;
}

void TreeView::SetVisibleCheckBtn( bool _IsVisibled )
{
	m_bVisibleCheckBtn = _IsVisibled;
	int nCount = this->getCount();
	for (int nIndex = 0;nIndex < nCount; ++nIndex) {
		TreeNode* pItem = static_cast<TreeNode*>(this->getItem(nIndex));
		pItem->GetCheckBox()->SetVisible(m_bVisibleCheckBtn);
	}
}

bool TreeView::GetVisibleCheckBtn()
{
	return m_bVisibleCheckBtn;
}

void TreeView::SetItemMinWidth( UINT _ItemMinWidth )
{
	m_uItemMinWidth = _ItemMinWidth;

	for (int nIndex = 0; nIndex < getCount(); ++nIndex) {
		TreeNode* pTreeNode = static_cast<TreeNode*>(getItem(nIndex));
		if (pTreeNode) {
			pTreeNode->SetMinWidth(GetItemMinWidth());
		}
	}
	Invalidate();
}

UINT TreeView::GetItemMinWidth()
{
	return m_uItemMinWidth;
}
	
void TreeView::SetItemTextColor( DWORD _dwItemTextColor )
{
	for (int nIndex = 0; nIndex < getCount(); ++nIndex) {
		TreeNode* pTreeNode = static_cast<TreeNode*>(getItem(nIndex));
		if (pTreeNode) {
			pTreeNode->SetItemTextColor(_dwItemTextColor);
		}
	}
}

void TreeView::SetItemHotTextColor( DWORD _dwItemHotTextColor )
{
	for (int nIndex = 0; nIndex < getCount(); ++nIndex) {
		TreeNode* pTreeNode = static_cast<TreeNode*>(getItem(nIndex));
		if (pTreeNode) {
			pTreeNode->SetItemHotTextColor(_dwItemHotTextColor);
		}
	}
}

void TreeView::SetSelItemTextColor( DWORD _dwSelItemTextColor )
{
	for (int nIndex = 0; nIndex < getCount(); ++nIndex) {
		TreeNode* pTreeNode = static_cast<TreeNode*>(getItem(nIndex));
		if (pTreeNode) {
			pTreeNode->SetSelItemTextColor(_dwSelItemTextColor);
		}
	}
}
		
void TreeView::SetSelItemHotTextColor( DWORD _dwSelHotItemTextColor )
{
	for (int nIndex = 0; nIndex < getCount(); ++nIndex) {
		TreeNode* pTreeNode = static_cast<TreeNode*>(getItem(nIndex));
		if (pTreeNode) {
			pTreeNode->SetSelItemHotTextColor(_dwSelHotItemTextColor);
		}
	}
}

void TreeView::setAttribute(const String& pstrName, const String& pstrValue)
{
	if (pstrName == "visiblefolderbtn") {
		SetVisibleFolderBtn(pstrValue == "true");
	}
	else if (pstrName == "visiblecheckbtn") {
		SetVisibleCheckBtn(pstrValue == "true");
	}
	else if (pstrName == "itemminwidth") {
		SetItemMinWidth((UINT)pstrValue.getIntValue());
	}
	else if (pstrName == "itemtextcolor") {
		SetItemTextColor((uint32_t)pstrValue.getHexValue32());
	}
	else if (pstrName == "itemhottextcolor") {
		SetItemHotTextColor((uint32_t)pstrValue.getHexValue32());
	}
	else if(pstrName == "selitemtextcolor") {
		SetSelItemTextColor((uint32_t)pstrValue.getHexValue32());
	}
	else if (pstrName == "selitemhottextcolor") {
		SetSelItemHotTextColor((uint32_t)pstrValue.getHexValue32());
	}
	else {
		List::setAttribute(pstrName, pstrValue);
	}
}

}

#endif // ZGUI_USE_LIST && ZGUI_USE_TREEVIEW