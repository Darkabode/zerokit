#include "zgui.h"

#ifdef ZGUI_USE_TILELAYOUT

namespace zgui
{
	CTileLayoutUI::CTileLayoutUI() : m_nColumns(1)
	{
		m_szItem.cx = m_szItem.cy = 0;
	}

	LPCTSTR CTileLayoutUI::GetClass() const
	{
		return _T("TileLayoutUI");
	}

	LPVOID CTileLayoutUI::GetInterface(LPCTSTR pstrName)
	{
		if( lstrcmp(pstrName, DUI_CTR_TILELAYOUT) == 0 ) return static_cast<CTileLayoutUI*>(this);
		return CContainerUI::GetInterface(pstrName);
	}

	SIZE CTileLayoutUI::GetItemSize() const
	{
		return m_szItem;
	}

	void CTileLayoutUI::SetItemSize(SIZE szItem)
	{
		if( m_szItem.cx != szItem.cx || m_szItem.cy != szItem.cy ) {
			m_szItem = szItem;
			NeedUpdate();
		}
	}

	int CTileLayoutUI::GetColumns() const
	{
		return m_nColumns;
	}

	void CTileLayoutUI::SetColumns(int nCols)
	{
		if( nCols <= 0 ) return;
		m_nColumns = nCols;
		NeedUpdate();
	}

	void CTileLayoutUI::SetAttribute(const String& pstrName, const String& pstrValue)
	{
		if (pstrName == "itemsize") {
			SIZE szItem = { 0 };
            if (Helper::splitString(pstrValue, ",", String::empty, szItem.cx, szItem.cy)) {
                SetItemSize(szItem);
            }
		}
        else if (pstrName == "columns") {
            SetColumns(pstrValue.getIntValue());
        }
        else {
            CContainerUI::SetAttribute(pstrName, pstrValue);
        }
	}

	void CTileLayoutUI::SetPos(RECT rc)
	{
		CControlUI::SetPos(rc);
		rc = _rcItem;

		// Adjust for inset
		rc.left += _rcInset.left;
		rc.top += _rcInset.top;
		rc.right -= _rcInset.right;
		rc.bottom -= _rcInset.bottom;

		if( m_items.GetSize() == 0) {
			ProcessScrollBar(rc, 0, 0);
			return;
		}

		if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) rc.right -= _pVerticalScrollBar->GetFixedWidth();
		if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) rc.bottom -= _pHorizontalScrollBar->GetFixedHeight();

		// Position the elements
		if( m_szItem.cx > 0 ) m_nColumns = (rc.right - rc.left) / m_szItem.cx;
		if( m_nColumns == 0 ) m_nColumns = 1;

		int cyNeeded = 0;
		int cxWidth = (rc.right - rc.left) / m_nColumns;
		if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) 
			cxWidth = (rc.right - rc.left + _pHorizontalScrollBar->GetScrollRange() ) / m_nColumns; ;

		int cyHeight = 0;
		int iCount = 0;
		POINT ptTile = { rc.left, rc.top };
		if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) {
			ptTile.y -= _pVerticalScrollBar->GetScrollPos();
		}
		int iPosX = rc.left;
		if( _pHorizontalScrollBar && _pHorizontalScrollBar->IsVisible() ) {
			iPosX -= _pHorizontalScrollBar->GetScrollPos();
			ptTile.x = iPosX;
		}
		for( int it1 = 0; it1 < m_items.GetSize(); it1++ ) {
			CControlUI* pControl = static_cast<CControlUI*>(m_items[it1]);
			if( !pControl->IsVisible() ) continue;
			if( pControl->IsFloat() ) {
				SetFloatPos(it1);
				continue;
			}

			// Determine size
			RECT rcTile = { ptTile.x, ptTile.y, ptTile.x + cxWidth, ptTile.y };
			if( (iCount % m_nColumns) == 0 )
			{
				int iIndex = iCount;
				for( int it2 = it1; it2 < m_items.GetSize(); it2++ ) {
					CControlUI* pLineControl = static_cast<CControlUI*>(m_items[it2]);
					if( !pLineControl->IsVisible() ) continue;
					if( pLineControl->IsFloat() ) continue;

					RECT rcPadding = pLineControl->GetPadding();
					SIZE szAvailable = { rcTile.right - rcTile.left - rcPadding.left - rcPadding.right, 9999 };
					if( iIndex == iCount || (iIndex + 1) % m_nColumns == 0 ) {
						szAvailable.cx -= m_iChildPadding / 2;
					}
					else {
						szAvailable.cx -= m_iChildPadding;
					}

					if( szAvailable.cx < pControl->GetMinWidth() ) szAvailable.cx = pControl->GetMinWidth();
					if( szAvailable.cx > pControl->GetMaxWidth() ) szAvailable.cx = pControl->GetMaxWidth();

					SIZE szTile = pLineControl->EstimateSize(szAvailable);
					if( szTile.cx < pControl->GetMinWidth() ) szTile.cx = pControl->GetMinWidth();
					if( szTile.cx > pControl->GetMaxWidth() ) szTile.cx = pControl->GetMaxWidth();
					if( szTile.cy < pControl->GetMinHeight() ) szTile.cy = pControl->GetMinHeight();
					if( szTile.cy > pControl->GetMaxHeight() ) szTile.cy = pControl->GetMaxHeight();

					cyHeight = MAX(cyHeight, szTile.cy + rcPadding.top + rcPadding.bottom);
					if( (++iIndex % m_nColumns) == 0) break;
				}
			}

			RECT rcPadding = pControl->GetPadding();

			rcTile.left += rcPadding.left + m_iChildPadding / 2;
			rcTile.right -= rcPadding.right + m_iChildPadding / 2;
			if( (iCount % m_nColumns) == 0 ) {
				rcTile.left -= m_iChildPadding / 2;
			}

			if( ( (iCount + 1) % m_nColumns) == 0 ) {
				rcTile.right += m_iChildPadding / 2;
			}

			// Set position
			rcTile.top = ptTile.y + rcPadding.top;
			rcTile.bottom = ptTile.y + cyHeight;

			SIZE szAvailable = { rcTile.right - rcTile.left, rcTile.bottom - rcTile.top };
			SIZE szTile = pControl->EstimateSize(szAvailable);
			if( szTile.cx == 0 ) szTile.cx = szAvailable.cx;
			if( szTile.cy == 0 ) szTile.cy = szAvailable.cy;
			if( szTile.cx < pControl->GetMinWidth() ) szTile.cx = pControl->GetMinWidth();
			if( szTile.cx > pControl->GetMaxWidth() ) szTile.cx = pControl->GetMaxWidth();
			if( szTile.cy < pControl->GetMinHeight() ) szTile.cy = pControl->GetMinHeight();
			if( szTile.cy > pControl->GetMaxHeight() ) szTile.cy = pControl->GetMaxHeight();
			RECT rcPos = {(rcTile.left + rcTile.right - szTile.cx) / 2, (rcTile.top + rcTile.bottom - szTile.cy) / 2,
				(rcTile.left + rcTile.right - szTile.cx) / 2 + szTile.cx, (rcTile.top + rcTile.bottom - szTile.cy) / 2 + szTile.cy};
			pControl->SetPos(rcPos);

			if( (++iCount % m_nColumns) == 0 ) {
				ptTile.x = iPosX;
				ptTile.y += cyHeight + m_iChildPadding;
				cyHeight = 0;
			}
			else {
				ptTile.x += cxWidth;
			}
			cyNeeded = rcTile.bottom - rc.top;
			if( _pVerticalScrollBar && _pVerticalScrollBar->IsVisible() ) cyNeeded += _pVerticalScrollBar->GetScrollPos();
		}

		// Process the scrollbar
		ProcessScrollBar(rc, 0, cyNeeded);
	}
}

#endif // ZGUI_USE_TILELAYOUT
