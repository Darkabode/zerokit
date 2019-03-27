#ifndef __ZGUI_SCROLLBAR_H_
#define __ZGUI_SCROLLBAR_H_

#ifdef ZGUI_USE_SCROLLBAR

namespace zgui
{
	class ScrollBar : public Control
	{
	public:
		ScrollBar();

		const String& getClass() const;
		LPVOID getInterface(const String& name);

		Container* GetOwner() const;
		void SetOwner(Container* pOwner);

		void SetVisible(bool bVisible = true, const bool needUpdate = true);
		void SetEnabled(bool bEnable = true);
		void SetFocus();

		bool IsHorizontal();
		void SetHorizontal(bool bHorizontal = true);
		int GetScrollRange() const;
		void SetScrollRange(int nRange);
		int GetScrollPos() const;
		void SetScrollPos(int nPos);
		int GetLineSize() const;
		void SetLineSize(int nSize);

		bool GetShowButton1();
		void SetShowButton1(bool bShow);
		const String& GetButton1NormalImage();
		void SetButton1NormalImage(const String& pStrImage);
		const String& GetButton1HotImage();
		void SetButton1HotImage(const String& pStrImage);
		const String& GetButton1PushedImage();
		void SetButton1PushedImage(const String& pStrImage);
		const String& GetButton1DisabledImage();
		void SetButton1DisabledImage(const String& pStrImage);

		bool GetShowButton2();
		void SetShowButton2(bool bShow);
		const String& GetButton2NormalImage();
		void SetButton2NormalImage(const String& pStrImage);
		const String& GetButton2HotImage();
		void SetButton2HotImage(const String& pStrImage);
		const String& GetButton2PushedImage();
		void SetButton2PushedImage(const String& pStrImage);
		const String& GetButton2DisabledImage();
		void SetButton2DisabledImage(const String& pStrImage);

		const String& GetThumbNormalImage();
		void SetThumbNormalImage(const String& pStrImage);
		const String& GetThumbHotImage();
		void SetThumbHotImage(const String& pStrImage);
		const String& GetThumbPushedImage();
		void SetThumbPushedImage(const String& pStrImage);
		const String& GetThumbDisabledImage();
		void SetThumbDisabledImage(const String& pStrImage);

		const String& GetRailNormalImage();
		void SetRailNormalImage(const String& pStrImage);
		const String& GetRailHotImage();
		void SetRailHotImage(const String& pStrImage);
		const String& GetRailPushedImage();
		void SetRailPushedImage(const String& pStrImage);
		const String& GetRailDisabledImage();
		void SetRailDisabledImage(const String& pStrImage);

		const String& GetBkNormalImage();
		void SetBkNormalImage(const String& pStrImage);
		const String& GetBkHotImage();
		void SetBkHotImage(const String& pStrImage);
		const String& GetBkPushedImage();
		void SetBkPushedImage(const String& pStrImage);
		const String& GetBkDisabledImage();
		void SetBkDisabledImage(const String& pStrImage);

		void SetPos(RECT rc);
		void DoEvent(TEventUI& event);
		void setAttribute(const String& pstrName, const String& pstrValue);

		void DoPaint(HDC hDC, const RECT& rcPaint);

		void PaintBk(HDC hDC);
		void PaintButton1(HDC hDC);
		void PaintButton2(HDC hDC);
		void PaintThumb(HDC hDC);
		void PaintRail(HDC hDC);

	protected:

		enum
		{ 
			DEFAULT_SCROLLBAR_SIZE = 16,
			DEFAULT_TIMERID = 10,
		};

		bool _bHorizontal;
		int m_nRange;
		int m_nScrollPos;
		int m_nLineSize;
		Container* _pOwner;
		POINT ptLastMouse;
		int m_nLastScrollPos;
		int m_nLastScrollOffset;
		int m_nScrollRepeatDelay;

		String m_sBkNormalImage;
		String m_sBkHotImage;
		String m_sBkPushedImage;
		String m_sBkDisabledImage;

		bool m_bShowButton1;
		RECT m_rcButton1;
		UINT m_uButton1State;
		String _button1NormalImageName;
		String m_sButton1HotImage;
		String m_sButton1PushedImage;
		String m_sButton1DisabledImage;

		bool m_bShowButton2;
		RECT m_rcButton2;
		UINT m_uButton2State;
		String m_sButton2NormalImage;
		String m_sButton2HotImage;
		String m_sButton2PushedImage;
		String m_sButton2DisabledImage;

		RECT m_rcThumb;
		UINT m_uThumbState;
		String m_sThumbNormalImage;
		String m_sThumbHotImage;
		String m_sThumbPushedImage;
		String m_sThumbDisabledImage;

		String m_sRailNormalImage;
		String m_sRailHotImage;
		String m_sRailPushedImage;
		String m_sRailDisabledImage;

		String _imageModifyName;

		private:
			static const String CLASS_NAME;
	};
}

#endif // ZGUI_USE_SCROLLBAR

#endif // __ZGUI_SCROLLBAR_H_