#ifndef __ZGUI_DATETIME_H_
#define __ZGUI_DATETIME_H_

#ifdef ZGUI_USE_DATETIME

namespace zgui
{
	class CDateTimeWnd;

	/// 时间选择控件
	class CDateTimeUI : public CLabelUI
	{
		friend class CDateTimeWnd;
	public:
		CDateTimeUI();
		const String& getClass() const;
		LPVOID getInterface(const String& name);

		SYSTEMTIME& GetTime();
		void SetTime(SYSTEMTIME* pst);

		void SetReadOnly(bool bReadOnly);
		bool IsReadOnly() const;

		void UpdateText();

		void DoEvent(TEventUI& event);

	protected:
		SYSTEMTIME m_sysTime;
		int        m_nDTUpdateFlag;
		bool       m_bReadOnly;

		CDateTimeWnd* m_pWindow;
	};
}

#endif // ZGUI_USE_DATETIME

#endif // __ZGUI_DATETIME_H_