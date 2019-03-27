#ifndef __ZGUI_TEXT_H_
#define __ZGUI_TEXT_H_

#ifdef ZGUI_USE_TEXT

namespace zgui
{
	class CTextUI : public CLabelUI
	{
	public:
		CTextUI();
		~CTextUI();

		LPCTSTR GetClass() const;
		UINT GetControlFlags() const;
		LPVOID GetInterface(LPCTSTR pstrName);

		String* GetLinkContent(int iIndex);

		void DoEvent(TEventUI& event);
		SIZE EstimateSize(SIZE szAvailable);

		void PaintText(HDC hDC);

	protected:
		enum { MAX_LINK = 8 };
		int m_nLinks;
		RECT m_rcLinks[MAX_LINK];
		String m_sLinks[MAX_LINK];
		int m_nHoverLink;
	};

} // namespace zgui

#endif // ZGUI_USE_TEXT

#endif // __ZGUI_TEXT_H_