#include "zgui.h"

#ifdef ZGUI_USE_SCROLLBAR

#ifdef ZGUI_USE_CONTAINER

namespace zgui
{
	CContainerUI::CContainerUI() :
    m_iChildPadding(0),
	m_bAutoDestroy(true),
	m_bDelayedDestroy(true),
	m_bMouseChildEnabled(true),
	_pVerticalScrollBar(NULL),
	_pHorizontalScrollBar(NULL),
	m_bScrollProcess(false)
	{
		::ZeroMemory(&_rcInset, sizeof(_rcInset));
	}

	CContainerUI::~CContainerUI()
	{
		m_bDelayedDestroy = false;
		RemoveAll();
        if (_pVerticalScrollBar) {
            delete _pVerticalScrollBar;
        }
        if (_pHorizontalScrollBar) {
            delete _pHorizontalScrollBar;
        }
	}

	LPCTSTR CContainerUI::GetClass() const
	{
		return _T("ContainerUI");
	}

	LPVOID CContainerUI::GetInterface(LPCTSTR pstrName)
	{
        if (lstrcmp(pstrName, _T("IContainer")) == 0) {
            return static_cast<IContainerUI*>(this);
        }
        else if (lstrcmp(pstrName, DUI_CTR_CONTAINER) == 0) {
            return static_cast<CContainerUI*>(this);
        }
		return CControlUI::GetInterface(pstrName);
	}

	CControlUI* CContainerUI::GetItemAt(int iIndex) const
	{
        if (iIndex < 0 || iIndex >= m_items.GetSize()) {
            return NULL;
        }
		return static_cast<CControlUI*>(m_items[iIndex]);
	}

	int CContainerUI::GetItemIndex(CControlUI* pControl) const
	{
		for ( int it = 0; it < m_items.GetSize(); ++it) {
			if (static_cast<CControlUI*>(m_items[it]) == pControl) {
				return it;
			}
		}

		return -1;
	}

	bool CContainerUI::SetItemIndex(CControlUI* pControl, int iIndex)
	{
		for (int it = 0; it < m_items.GetSize(); ++it) {
			if( static_cast<CControlUI*>(m_items[it]) == pControl ) {
				NeedUpdate();            
				m_items.Remove(it);
				return m_items.InsertAt(iIndex, pControl);
			}
		}

		return false;
	}

	int CContainerUI::GetCount() const
	{
		return m_items.GetSize();
	}

	bool CContainerUI::Add(CControlUI* pControl)
	{
        if (pControl == NULL) {
            return false;
        }

        if (_pManager != NULL) {
            _pManager->InitControls(pControl, this);
        }
		
        if (IsVisible()) {
            NeedUpdate();
        }
        else {
            pControl->SetInternVisible(false);
        }

		return m_items.Add(pControl);   
	}

	bool CContainerUI::AddAt(CControlUI* pControl, int iIndex)
	{
        if (pControl == NULL) {
            return false;
        }

        if (_pManager != NULL) {
            _pManager->InitControls(pControl, this);
        }
        if (IsVisible()) {
            NeedUpdate();
        }
        else {
            pControl->SetInternVisible(false);
        }

		return m_items.InsertAt(iIndex, pControl);
	}

	bool CContainerUI::Remove(CControlUI* pControl)
	{
		if( pControl == NULL) return false;

		for (int it = 0; it < m_items.GetSize(); ++it) {
			if (static_cast<CControlUI*>(m_items[it]) == pControl) {
				NeedUpdate();
				if (m_bAutoDestroy) {
                    if (m_bDelayedDestroy && _pManager) {
                        _pManager->AddDelayedCleanup(pControl);
                    }
                    else {
                        delete pControl;
                    }
				}
				return m_items.Remove(it);
			}
		}
		return false;
	}

	bool CContainerUI::RemoveAt(int iIndex)
	{
		CControlUI* pControl = GetItemAt(iIndex);
		if (pControl != NULL) {
			return CContainerUI::Remove(pControl);
		}

		return false;
	}

	void CContainerUI::RemoveAll()
	{
		for (int it = 0; m_bAutoDestroy && it < m_items.GetSize(); ++it) {
            if (m_bDelayedDestroy && _pManager) {
                _pManager->AddDelayedCleanup(static_cast<CControlUI*>(m_items[it]));
            }
            else {
                delete static_cast<CControlUI*>(m_items[it]);
            }
		}
		m_items.Empty();
		NeedUpdate();
	}

	bool CContainerUI::IsAutoDestroy() const
	{
		return m_bAutoDestroy;
	}

	void CContainerUI::SetAutoDestroy(bool bAuto)
	{
		m_bAutoDestroy = bAuto;
	}

	bool CContainerUI::IsDelayedDestroy() const
	{
		return m_bDelayedDestroy;
	}

	void CContainerUI::SetDelayedDestroy(bool bDelayed)
	{
		m_bDelayedDestroy = bDelayed;
	}

	RECT CContainerUI::GetInset() const
	{
		return _rcInset;
	}

	void CContainerUI::SetInset(RECT& rcInset)
	{
		_rcInset = rcInset;
		NeedUpdate();
	}

	int CContainerUI::GetChildPadding() const
	{
		return m_iChildPadding;
	}

	void CContainerUI::SetChildPadding(int iPadding)
	{
		m_iChildPadding = iPadding;
		NeedUpdate();
	}

	bool CContainerUI::IsMouseChildEnabled() const
	{
		return m_bMouseChildEnabled;
	}

	void CContainerUI::SetMouseChildEnabled(bool bEnable)
	{
		m_bMouseChildEnabled = bEnable;
	}

	void CContainerUI::SetVisible(bool bVisible)
	{
        if (m_bVisible == bVisible) {
            return;
        }
		CControlUI::SetVisible(bVisible);
		for (int it = 0; it < m_items.GetSize(); ++it) {
			static_cast<CControlUI*>(m_items[it])->SetInternVisible(IsVisible());
		}
	}

	void CContainerUI::SetInternVisible(bool bVisible)
	{
		CControlUI::SetInternVisible(bVisible);
        if (m_items.IsEmpty()) {
            return;
        }
		for (int it = 0; it < m_items.GetSize(); ++it) {
			static_cast<CControlUI*>(m_items[it])->SetInternVisible(IsVisible());
		}
	}

	void CContainerUI::SetMouseEnabled(bool bEnabled)
	{
        if (_pVerticalScrollBar != NULL) {
            _pVerticalScrollBar->SetMouseEnabled(bEnabled);
        }
        if (_pHorizontalScrollBar != NULL) {
            _pHorizontalScrollBar->SetMouseEnabled(bEnabled);
        }
		CControlUI::SetMouseEnabled(bEnabled);
	}

	void CContainerUI::DoEvent(TEventUI& event)
	{
		if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
            if (_pParent != NULL) {
                _pParent->DoEvent(event);
            }
            else {
                CControlUI::DoEvent(event);
            }
			return;
		}

		if (event.Type == UIEVENT_SETFOCUS) {
			m_bFocused = true;
			return;
		}
		if (event.Type == UIEVENT_KILLFOCUS) {
			m_bFocused = false;
			return;
		}
		if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible() && _pVerticalScrollBar->IsEnabled()) {
			if (event.Type == UIEVENT_KEYDOWN) {
				switch (event.chKey) {
			        case VK_DOWN:
				        LineDown();
				        return;
			        case VK_UP:
				        LineUp();
				        return;
			        case VK_NEXT:
				        PageDown();
				        return;
			        case VK_PRIOR:
				        PageUp();
				        return;
			        case VK_HOME:
				        HomeUp();
				        return;
			        case VK_END:
				        EndDown();
				        return;
				}
			}
			else if (event.Type == UIEVENT_SCROLLWHEEL) {
				switch (LOWORD(event.wParam)) {
			        case SB_LINEUP:
				        LineUp();
				        return;
			        case SB_LINEDOWN:
				        LineDown();
				        return;
				}
			}
		}
		else if( _pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible() && _pHorizontalScrollBar->IsEnabled() ) {
			if( event.Type == UIEVENT_KEYDOWN ) 
			{
				switch( event.chKey ) {
			        case VK_DOWN:
				        LineRight();
				        return;
			        case VK_UP:
				        LineLeft();
				        return;
			        case VK_NEXT:
				        PageRight();
				        return;
			        case VK_PRIOR:
				        PageLeft();
				        return;
			        case VK_HOME:
				        HomeLeft();
				        return;
			        case VK_END:
				        EndRight();
				        return;
				}
			}
			else if( event.Type == UIEVENT_SCROLLWHEEL )
			{
				switch (LOWORD(event.wParam)) {
			        case SB_LINEUP:
				        LineLeft();
				        return;
			        case SB_LINEDOWN:
				        LineRight();
				        return;
				}
			}
		}
		CControlUI::DoEvent(event);
	}

	SIZE CContainerUI::GetScrollPos() const
	{
		SIZE sz = {0, 0};
        if (_pVerticalScrollBar && _pVerticalScrollBar->IsVisible()) {
            sz.cy = _pVerticalScrollBar->GetScrollPos();
        }
        if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
            sz.cx = _pHorizontalScrollBar->GetScrollPos();
        }

		return sz;
	}

	SIZE CContainerUI::GetScrollRange() const
	{
		SIZE sz = {0, 0};
        if (_pVerticalScrollBar && _pVerticalScrollBar->IsVisible()) {
            sz.cy = _pVerticalScrollBar->GetScrollRange();
        }
        if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
            sz.cx = _pHorizontalScrollBar->GetScrollRange();
        }
		return sz;
	}

	void CContainerUI::SetScrollPos(SIZE szPos)
	{
		int cx = 0;
		int cy = 0;
		if (_pVerticalScrollBar && _pVerticalScrollBar->IsVisible()) {
			int iLastScrollPos = _pVerticalScrollBar->GetScrollPos();
			_pVerticalScrollBar->SetScrollPos(szPos.cy);
			cy = _pVerticalScrollBar->GetScrollPos() - iLastScrollPos;
		}

		if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
			int iLastScrollPos = _pHorizontalScrollBar->GetScrollPos();
			_pHorizontalScrollBar->SetScrollPos(szPos.cx);
			cx = _pHorizontalScrollBar->GetScrollPos() - iLastScrollPos;
		}

        if (cx == 0 && cy == 0) {
            return;
        }

		RECT rcPos;
		for (int it2 = 0; it2 < m_items.GetSize(); ++it2) {
			CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
			if( !pControl->IsVisible() ) continue;
			if( pControl->IsFloat() ) continue;

			rcPos = pControl->GetPos();
			rcPos.left -= cx;
			rcPos.right -= cx;
			rcPos.top -= cy;
			rcPos.bottom -= cy;
			pControl->SetPos(rcPos);
		}

		Invalidate();
	}

	void CContainerUI::LineUp()
	{
		int cyLine = 8;
		if( _pManager ) cyLine = _pManager->GetDefaultFontInfo()->tm.tmHeight + 8;

		SIZE sz = GetScrollPos();
		sz.cy -= cyLine;
		SetScrollPos(sz);
	}

	void CContainerUI::LineDown()
	{
		int cyLine = 8;
		if( _pManager ) cyLine = _pManager->GetDefaultFontInfo()->tm.tmHeight + 8;

		SIZE sz = GetScrollPos();
		sz.cy += cyLine;
		SetScrollPos(sz);
	}

	void CContainerUI::PageUp()
	{
		SIZE sz = GetScrollPos();
		int iOffset = _rcItem.bottom - _rcItem.top - _rcInset.top - _rcInset.bottom;
		if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) iOffset -= _pHorizontalScrollBar->GetFixedHeight();
		sz.cy -= iOffset;
		SetScrollPos(sz);
	}

	void CContainerUI::PageDown()
	{
		SIZE sz = GetScrollPos();
		int iOffset = _rcItem.bottom - _rcItem.top - _rcInset.top - _rcInset.bottom;
		if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) iOffset -= _pHorizontalScrollBar->GetFixedHeight();
		sz.cy += iOffset;
		SetScrollPos(sz);
	}

	void CContainerUI::HomeUp()
	{
		SIZE sz = GetScrollPos();
		sz.cy = 0;
		SetScrollPos(sz);
	}

	void CContainerUI::EndDown()
	{
		SIZE sz = GetScrollPos();
		sz.cy = GetScrollRange().cy;
		SetScrollPos(sz);
	}

	void CContainerUI::LineLeft()
	{
		SIZE sz = GetScrollPos();
		sz.cx -= 8;
		SetScrollPos(sz);
	}

	void CContainerUI::LineRight()
	{
		SIZE sz = GetScrollPos();
		sz.cx += 8;
		SetScrollPos(sz);
	}

	void CContainerUI::PageLeft()
	{
		SIZE sz = GetScrollPos();
		int iOffset = _rcItem.right - _rcItem.left - _rcInset.left - _rcInset.right;
		if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) iOffset -= _pVerticalScrollBar->GetFixedWidth();
		sz.cx -= iOffset;
		SetScrollPos(sz);
	}

	void CContainerUI::PageRight()
	{
		SIZE sz = GetScrollPos();
		int iOffset = _rcItem.right - _rcItem.left - _rcInset.left - _rcInset.right;
		if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) iOffset -= _pVerticalScrollBar->GetFixedWidth();
		sz.cx += iOffset;
		SetScrollPos(sz);
	}

	void CContainerUI::HomeLeft()
	{
		SIZE sz = GetScrollPos();
		sz.cx = 0;
		SetScrollPos(sz);
	}

	void CContainerUI::EndRight()
	{
		SIZE sz = GetScrollPos();
		sz.cx = GetScrollRange().cx;
		SetScrollPos(sz);
	}

	void CContainerUI::EnableScrollBar(bool bEnableVertical, bool bEnableHorizontal)
	{
		if (bEnableVertical && !_pVerticalScrollBar) {
			_pVerticalScrollBar = new CScrollBarUI;
			_pVerticalScrollBar->SetOwner(this);
			_pVerticalScrollBar->SetManager(_pManager, NULL, false);
			if (_pManager != 0) {
				LPCTSTR pDefaultAttributes = _pManager->GetDefaultAttributeList(_T("VScrollBar"));
				if (pDefaultAttributes) {
					_pVerticalScrollBar->ApplyAttributeList(pDefaultAttributes);
				}
			}
		}
		else if (!bEnableVertical && _pVerticalScrollBar) {
			delete _pVerticalScrollBar;
			_pVerticalScrollBar = NULL;
		}

		if (bEnableHorizontal && !_pHorizontalScrollBar) {
			_pHorizontalScrollBar = new CScrollBarUI;
			_pHorizontalScrollBar->SetHorizontal(true);
			_pHorizontalScrollBar->SetOwner(this);
			_pHorizontalScrollBar->SetManager(_pManager, NULL, false);
			if ( _pManager ) {
				LPCTSTR pDefaultAttributes = _pManager->GetDefaultAttributeList(_T("HScrollBar"));
				if( pDefaultAttributes ) {
					_pHorizontalScrollBar->ApplyAttributeList(pDefaultAttributes);
				}
			}
		}
		else if( !bEnableHorizontal && _pHorizontalScrollBar ) {
			delete _pHorizontalScrollBar;
			_pHorizontalScrollBar = NULL;
		}

		NeedUpdate();
	}

	CScrollBarUI* CContainerUI::GetVerticalScrollBar() const
	{
		return _pVerticalScrollBar;
	}

	CScrollBarUI* CContainerUI::GetHorizontalScrollBar() const
	{
		return _pHorizontalScrollBar;
	}

	int CContainerUI::FindSelectable(int iIndex, bool bForward /*= true*/) const
	{
		// NOTE: This is actually a helper-function for the list/combo/ect controls
		//       that allow them to find the next enabled/available selectable item
		if( GetCount() == 0 ) return -1;
		iIndex = CLAMP(iIndex, 0, GetCount() - 1);
		if( bForward ) {
			for( int i = iIndex; i < GetCount(); i++ ) {
				if( GetItemAt(i)->GetInterface(_T("ListItem")) != NULL 
					&& GetItemAt(i)->IsVisible()
					&& GetItemAt(i)->IsEnabled() ) return i;
			}
			return -1;
		}
		else {
			for( int i = iIndex; i >= 0; --i ) {
				if( GetItemAt(i)->GetInterface(_T("ListItem")) != NULL 
					&& GetItemAt(i)->IsVisible()
					&& GetItemAt(i)->IsEnabled() ) return i;
			}
			return FindSelectable(0, true);
		}
	}

	void CContainerUI::SetPos(RECT rc)
	{
		CControlUI::SetPos(rc);
		if( m_items.IsEmpty() ) return;
		rc.left += _rcInset.left;
		rc.top += _rcInset.top;
		rc.right -= _rcInset.right;
		rc.bottom -= _rcInset.bottom;

		for( int it = 0; it < m_items.GetSize(); it++ ) {
			CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
			if( !pControl->IsVisible() ) continue;
			if( pControl->IsFloat() ) {
				SetFloatPos(it);
			}
			else {
				pControl->SetPos(rc); // 所有非float子控件放大到整个客户区
			}
		}
	}

	void CContainerUI::SetAttribute(const String& pstrName, const String& pstrValue)
	{
		if (pstrName == "inset") {
			RECT rcInset = { 0 };
            if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcInset.left, (int&)rcInset.top, (int&)rcInset.right, (int&)rcInset.bottom)) {
                SetInset(rcInset);
            }
		}
        else if (pstrName == "mousechild") {
            SetMouseChildEnabled(pstrValue == "true");
        }
		else if (pstrName == "vscrollbar") {
			EnableScrollBar(pstrValue == "true", GetHorizontalScrollBar() != NULL);
		}
		else if (pstrName == "vscrollbarstyle") {
			EnableScrollBar(true, GetHorizontalScrollBar() != NULL);
            if (GetVerticalScrollBar()) {
                GetVerticalScrollBar()->ApplyAttributeList(pstrValue);
            }
		}
		else if (pstrName == "hscrollbar") {
			EnableScrollBar(GetVerticalScrollBar() != NULL, pstrValue == "true");
		}
		else if (pstrName == "hscrollbarstyle") {
			EnableScrollBar(GetVerticalScrollBar() != NULL, true);
            if (GetHorizontalScrollBar()) {
                GetHorizontalScrollBar()->ApplyAttributeList(pstrValue);
            }
		}
        else if (pstrName == "childpadding") {
            SetChildPadding(pstrValue.getIntValue());
        }
        else {
            CControlUI::SetAttribute(pstrName, pstrValue);
        }
	}

	void CContainerUI::SetManager(CPaintManagerUI* pManager, CControlUI* pParent, bool bInit)
	{
		for (int it = 0; it < m_items.GetSize(); ++it) {
			static_cast<CControlUI*>(m_items[it])->SetManager(pManager, this, bInit);
		}

        if (_pVerticalScrollBar != NULL) {
            _pVerticalScrollBar->SetManager(pManager, this, bInit);
        }
        if (_pHorizontalScrollBar != NULL) {
            _pHorizontalScrollBar->SetManager(pManager, this, bInit);
        }

		CControlUI::SetManager(pManager, pParent, bInit);
	}

	CControlUI* CContainerUI::FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags)
	{
		// Check if this guy is valid
		if( (uFlags & UIFIND_VISIBLE) != 0 && !IsVisible() ) return NULL;
		if( (uFlags & UIFIND_ENABLED) != 0 && !IsEnabled() ) return NULL;
		if( (uFlags & UIFIND_HITTEST) != 0 ) {
			if( !::PtInRect(&_rcItem, *(static_cast<LPPOINT>(pData))) ) return NULL;
			if( !m_bMouseChildEnabled ) {
				CControlUI* pResult = NULL;
				if( _pVerticalScrollBar != NULL ) pResult = _pVerticalScrollBar->FindControl(Proc, pData, uFlags);
				if( pResult == NULL && _pHorizontalScrollBar != NULL ) pResult = _pHorizontalScrollBar->FindControl(Proc, pData, uFlags);
				if( pResult == NULL ) pResult = CControlUI::FindControl(Proc, pData, uFlags);
				return pResult;
			}
		}

		CControlUI* pResult = NULL;
		if( _pVerticalScrollBar != NULL ) pResult = _pVerticalScrollBar->FindControl(Proc, pData, uFlags);
		if( pResult == NULL && _pHorizontalScrollBar != NULL ) pResult = _pHorizontalScrollBar->FindControl(Proc, pData, uFlags);
		if( pResult != NULL ) return pResult;

		if( (uFlags & UIFIND_ME_FIRST) != 0 ) {
			CControlUI* pControl = CControlUI::FindControl(Proc, pData, uFlags);
			if( pControl != NULL ) return pControl;
		}
		RECT rc = _rcItem;
		rc.left += _rcInset.left;
		rc.top += _rcInset.top;
		rc.right -= _rcInset.right;
		rc.bottom -= _rcInset.bottom;
		if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) rc.right -= _pVerticalScrollBar->GetFixedWidth();
		if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();
		if( (uFlags & UIFIND_TOP_FIRST) != 0 ) {
			for( int it = m_items.GetSize() - 1; it >= 0; it-- ) {
				CControlUI* pControl = static_cast<CControlUI*>(m_items[it])->FindControl(Proc, pData, uFlags);
				if( pControl != NULL ) {
					if( (uFlags & UIFIND_HITTEST) != 0 && !pControl->IsFloat() && !::PtInRect(&rc, *(static_cast<LPPOINT>(pData))) )
						continue;
					else 
						return pControl;
				}            
			}
		}
		else {
			for( int it = 0; it < m_items.GetSize(); it++ ) {
				CControlUI* pControl = static_cast<CControlUI*>(m_items[it])->FindControl(Proc, pData, uFlags);
				if( pControl != NULL ) {
					if( (uFlags & UIFIND_HITTEST) != 0 && !pControl->IsFloat() && !::PtInRect(&rc, *(static_cast<LPPOINT>(pData))) )
						continue;
					else 
						return pControl;
				} 
			}
		}

		if( pResult == NULL && (uFlags & UIFIND_ME_FIRST) == 0 ) pResult = CControlUI::FindControl(Proc, pData, uFlags);
		return pResult;
	}

	void CContainerUI::DoPaint(HDC hDC, const RECT& rcPaint)
	{
		RECT rcTemp = {0};
        if (!::IntersectRect(&rcTemp, &rcPaint, &_rcItem)) {
            return;
        }

		CRenderClip clip;
		CRenderClip::GenerateClip(hDC, rcTemp, clip);
		CControlUI::DoPaint(hDC, rcPaint); 

		if (m_items.GetSize() > 0) { 
			RECT rc = _rcItem;
			rc.left += _rcInset.left;
			rc.top += _rcInset.top;  
			rc.right -= _rcInset.right; 
			rc.bottom -= _rcInset.bottom;
            if (_pVerticalScrollBar && _pVerticalScrollBar->IsVisible()) {
                rc.right -= _pVerticalScrollBar->GetFixedWidth();
            } 
            if (_pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible()) {
                rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();
            }
			if (!::IntersectRect(&rcTemp, &rcPaint, &rc)) {
				for (int it = 0; it < m_items.GetSize(); ++it) {
					CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
                    if (!pControl->IsVisible()) { 
                        continue;
                    }
                    if (!::IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos())) {
                        continue;
                    }
					if (pControl ->IsFloat() ) {
                        if (!::IntersectRect(&rcTemp, &_rcItem, &pControl->GetPos())) {
                            continue;
                        }
						pControl->DoPaint(hDC, rcPaint);
					}
				}
			}
			else {
				CRenderClip childClip;
				CRenderClip::GenerateClip(hDC, rcTemp, childClip);
				for (int it = 0; it < m_items.GetSize(); ++it) {
					CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
                    if (!pControl->IsVisible()) {
                        continue;
                    }
                    if (!::IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos())) { 
                        continue;
                    }
					if (pControl ->IsFloat()) {
                        if (!::IntersectRect(&rcTemp, &_rcItem, &pControl->GetPos())) {
                            continue;
                        }
						CRenderClip::UseOldClipBegin(hDC, childClip);
						pControl->DoPaint(hDC, rcPaint);
						CRenderClip::UseOldClipEnd(hDC, childClip);
					} 
					else { 
                        if (!::IntersectRect(&rcTemp, &rc, &pControl->GetPos())) {
                            continue;
                        }
						pControl->DoPaint(hDC, rcPaint);
					}
				} 
			}
		}

		if (_pVerticalScrollBar != NULL && _pVerticalScrollBar->IsVisible()) {
			if (::IntersectRect(&rcTemp, &rcPaint, &_pVerticalScrollBar->GetPos())) {
				_pVerticalScrollBar->DoPaint(hDC, rcPaint);
			}
		}

		if (_pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible()) {
			if (::IntersectRect(&rcTemp, &rcPaint, &_pHorizontalScrollBar->GetPos())) {
				_pHorizontalScrollBar->DoPaint(hDC, rcPaint);
			}
		}
	}

	void CContainerUI::SetFloatPos(int iIndex)
	{
		// 因为CControlUI::SetPos对float的操作影响，这里不能对float组件添加滚动条的影响
		if( iIndex < 0 || iIndex >= m_items.GetSize() ) return;

		CControlUI* pControl = static_cast<CControlUI*>(m_items[iIndex]);

		if( !pControl->IsVisible() ) return;
		if( !pControl->IsFloat() ) return;

		SIZE szXY = pControl->GetFixedXY();
		SIZE sz = {pControl->GetFixedWidth(), pControl->GetFixedHeight()};
		RECT rcCtrl = { 0 };
		if( szXY.cx >= 0 ) {
			rcCtrl.left = _rcItem.left + szXY.cx;
			rcCtrl.right = _rcItem.left + szXY.cx + sz.cx;
		}
		else {
			rcCtrl.left = _rcItem.right + szXY.cx - sz.cx;
			rcCtrl.right = _rcItem.right + szXY.cx;
		}
		if( szXY.cy >= 0 ) {
			rcCtrl.top = _rcItem.top + szXY.cy;
			rcCtrl.bottom = _rcItem.top + szXY.cy + sz.cy;
		}
		else {
			rcCtrl.top = _rcItem.bottom + szXY.cy - sz.cy;
			rcCtrl.bottom = _rcItem.bottom + szXY.cy;
		}
		if( pControl->IsRelativePos() )
		{
			TRelativePosUI tRelativePos = pControl->GetRelativePos();
			SIZE szParent = {_rcItem.right-_rcItem.left,_rcItem.bottom-_rcItem.top};
			if(tRelativePos.szParent.cx != 0)
			{
				int nIncrementX = szParent.cx-tRelativePos.szParent.cx;
				int nIncrementY = szParent.cy-tRelativePos.szParent.cy;
				rcCtrl.left += (nIncrementX*tRelativePos.nMoveXPercent/100);
				rcCtrl.top += (nIncrementY*tRelativePos.nMoveYPercent/100);
				rcCtrl.right = rcCtrl.left+sz.cx+(nIncrementX*tRelativePos.nZoomXPercent/100);
				rcCtrl.bottom = rcCtrl.top+sz.cy+(nIncrementY*tRelativePos.nZoomYPercent/100);
			}
			pControl->SetRelativeParentSize(szParent);
		}
		pControl->SetPos(rcCtrl);
	}

	void CContainerUI::ProcessScrollBar(RECT rc, int cxRequired, int cyRequired)
	{
		if( _pHorizontalScrollBar != NULL && _pHorizontalScrollBar->IsVisible() ) {
			RECT rcScrollBarPos = { rc.left, rc.bottom, rc.right, rc.bottom + _pHorizontalScrollBar->GetFixedHeight()};
			_pHorizontalScrollBar->SetPos(rcScrollBarPos);
		}

		if( _pVerticalScrollBar == NULL ) return;

		if( cyRequired > rc.bottom - rc.top && !_pVerticalScrollBar->IsVisible() ) {
			_pVerticalScrollBar->SetVisible(true);
			_pVerticalScrollBar->SetScrollRange(cyRequired - (rc.bottom - rc.top));
			_pVerticalScrollBar->SetScrollPos(0);
			m_bScrollProcess = true;
			SetPos(_rcItem);
			m_bScrollProcess = false;
			return;
		}
		// No scrollbar required
		if( !_pVerticalScrollBar->IsVisible() ) return;

		// Scroll not needed anymore?
		int cyScroll = cyRequired - (rc.bottom - rc.top);
		if( cyScroll <= 0 && !m_bScrollProcess) {
			_pVerticalScrollBar->SetVisible(false);
			_pVerticalScrollBar->SetScrollPos(0);
			_pVerticalScrollBar->SetScrollRange(0);
			SetPos(_rcItem);
		}
		else
		{
			RECT rcScrollBarPos = { rc.right, rc.top, rc.right + _pVerticalScrollBar->GetFixedWidth(), rc.bottom };
			_pVerticalScrollBar->SetPos(rcScrollBarPos);

			if( _pVerticalScrollBar->GetScrollRange() != cyScroll ) {
				int iScrollPos = _pVerticalScrollBar->GetScrollPos();
				_pVerticalScrollBar->SetScrollRange(::abs(cyScroll));
				if( _pVerticalScrollBar->GetScrollRange() == 0 ) {
					_pVerticalScrollBar->SetVisible(false);
					_pVerticalScrollBar->SetScrollPos(0);
				}
				if( iScrollPos > _pVerticalScrollBar->GetScrollPos() ) {
					SetPos(_rcItem);
				}
			}
		}
	}

	bool CContainerUI::SetSubControlText( LPCTSTR pstrSubControlName,LPCTSTR pstrText )
	{
		CControlUI* pSubControl=NULL;
		pSubControl=this->FindSubControl(pstrSubControlName);
		if (pSubControl!=NULL)
		{
			pSubControl->SetText(pstrText);
			return TRUE;
		}
		else
			return FALSE;
	}

	bool CContainerUI::SetSubControlFixedHeight( LPCTSTR pstrSubControlName,int cy )
	{
		CControlUI* pSubControl=NULL;
		pSubControl=this->FindSubControl(pstrSubControlName);
		if (pSubControl!=NULL)
		{
			pSubControl->SetFixedHeight(cy);
			return TRUE;
		}
		else
			return FALSE;
	}

	bool CContainerUI::SetSubControlFixedWdith( LPCTSTR pstrSubControlName,int cx )
	{
		CControlUI* pSubControl=NULL;
		pSubControl=this->FindSubControl(pstrSubControlName);
		if (pSubControl!=NULL)
		{
			pSubControl->SetFixedWidth(cx);
			return TRUE;
		}
		else
			return FALSE;
	}

	bool CContainerUI::SetSubControlUserData( LPCTSTR pstrSubControlName,LPCTSTR pstrText )
	{
		CControlUI* pSubControl=NULL;
		pSubControl=this->FindSubControl(pstrSubControlName);
		if (pSubControl!=NULL)
		{
			pSubControl->SetUserData(pstrText);
			return TRUE;
		}
		else
			return FALSE;
	}

	String CContainerUI::GetSubControlText(LPCTSTR pstrSubControlName)
	{
		CControlUI* pSubControl=NULL;
		pSubControl=this->FindSubControl(pstrSubControlName);
        if (pSubControl==NULL) {
			return "";
        }
        else {
			return pSubControl->GetText();
        }
	}

	int CContainerUI::GetSubControlFixedHeight( LPCTSTR pstrSubControlName )
	{
		CControlUI* pSubControl=NULL;
		pSubControl=this->FindSubControl(pstrSubControlName);
		if (pSubControl==NULL)
			return -1;
		else
			return pSubControl->GetFixedHeight();
	}

	int CContainerUI::GetSubControlFixedWdith( LPCTSTR pstrSubControlName )
	{
		CControlUI* pSubControl=NULL;
		pSubControl=this->FindSubControl(pstrSubControlName);
		if (pSubControl==NULL)
			return -1;
		else
			return pSubControl->GetFixedWidth();
	}

	String CContainerUI::GetSubControlUserData( LPCTSTR pstrSubControlName )
	{
		CControlUI* pSubControl=NULL;
		pSubControl=this->FindSubControl(pstrSubControlName);
        if (pSubControl==NULL) {
			return "";
        }
        else {
			return pSubControl->GetUserData();
        }
	}

	CControlUI* CContainerUI::FindSubControl( LPCTSTR pstrSubControlName )
	{
		CControlUI* pSubControl=NULL;
		pSubControl=static_cast<CControlUI*>(GetManager()->FindSubControlByName(this,pstrSubControlName));
		return pSubControl;
	}

} // namespace zgui

#endif // ZGUI_USE_CONTAINER

#endif // ZGUI_USE_SCROLLBAR