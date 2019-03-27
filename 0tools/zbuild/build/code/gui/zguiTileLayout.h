#ifndef __ZGUI_TILELAYOUT_H_
#define __ZGUI_TILELAYOUT_H_

#ifdef ZGUI_USE_TILELAYOUT

namespace zgui
{
	class CTileLayoutUI : public CContainerUI
	{
	public:
		CTileLayoutUI();

		LPCTSTR GetClass() const;
		LPVOID GetInterface(LPCTSTR pstrName);

		void SetPos(RECT rc);

		SIZE GetItemSize() const;
		void SetItemSize(SIZE szItem);
		int GetColumns() const;
		void SetColumns(int nCols);

		void SetAttribute(const String& pstrName, const String& pstrValue);

	protected:
		SIZE m_szItem;
		int m_nColumns;
	};
}

#endif // ZGUI_USE_TILELAYOUT

#endif // __UITILELAYOUT_H__
