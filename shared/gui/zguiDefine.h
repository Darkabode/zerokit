#pragma once

#include "zgui.h"


namespace zgui
{

enum ZGuiSig
{
	ZGuiSig_end = 0, // [marks end of message map]
	ZGuiSig_lwl,     // LRESULT (WPARAM, LPARAM)
	ZGuiSig_vn,      // void (TNotifyUI)
};

class Control;

// Structure for notifications to the outside world
struct TNotifyUI 
{
    TNotifyUI() :
    pSender(0)
    {

    }

    ~TNotifyUI()
    {

    }

    String sType;
    String sVirtualWnd;
    Control* pSender;
    DWORD dwTimestamp;
    POINT ptMouse;
    WPARAM wParam;
    LPARAM lParam;
};

class CNotifyPump;
typedef void (CNotifyPump::*ZGUI_PMSG)(TNotifyUI& msg);  //指针类型


union ZGuiMessageMapFunctions
{
	ZGUI_PMSG pfn;   // generic member function pointer
	LRESULT (CNotifyPump::*pfn_Notify_lwl)(WPARAM, LPARAM);
	void (CNotifyPump::*pfn_Notify_vn)(TNotifyUI&);
};

//定义所有消息类型
//////////////////////////////////////////////////////////////////////////

#define ZGUI_MSGTYPE_MENU			("menu")
#define ZGUI_MSGTYPE_LINK			("link")

#define ZGUI_MSGTYPE_TIMER			("timer")
#define ZGUI_MSGTYPE_CLICK			("click")

#define ZGUI_MSGTYPE_RETURN			("return")
#define ZGUI_MSGTYPE_SCROLL			("scroll")

#define ZGUI_MSGTYPE_DROPDOWN		("dropdown")
#define ZGUI_MSGTYPE_SETFOCUS		("setfocus")

#define ZGUI_MSGTYPE_KILLFOCUS		("killfocus")
#define ZGUI_MSGTYPE_ITEMCLICK		("itemclick")
#define ZGUI_MSGTYPE_TABSELECT		("tabselect")

#define ZGUI_MSGTYPE_ITEMSELECT		("itemselect")
#define ZGUI_MSGTYPE_ITEMEXPAND		("itemexpand")
#define ZGUI_MSGTYPE_WINDOWINIT		("windowinit")
#define ZGUI_MSGTYPE_BUTTONDOWN		("buttondown")
#define ZGUI_MSGTYPE_MOUSEENTER		("mouseenter")
#define ZGUI_MSGTYPE_MOUSELEAVE		("mouseleave")

#define ZGUI_MSGTYPE_TEXTCHANGED	("textchanged")
#define ZGUI_MSGTYPE_HEADERCLICK	("headerclick")
#define ZGUI_MSGTYPE_ITEMDBCLICK	("itemdbclick")
#define ZGUI_MSGTYPE_SHOWACTIVEX	("showactivex")

#define ZGUI_MSGTYPE_ITEMCOLLAPSE	("itemcollapse")
#define ZGUI_MSGTYPE_ITEMACTIVATE	("itemactivate")
#define ZGUI_MSGTYPE_VALUECHANGED	("valuechanged")

#define ZGUI_MSGTYPE_SELECTCHANGED	("selectchanged")



//////////////////////////////////////////////////////////////////////////



struct ZGUI_MSGMAP_ENTRY;
struct ZGUI_MSGMAP
{
    const ZGUI_MSGMAP* pBaseMap;
    const ZGUI_MSGMAP_ENTRY* lpEntries;
};

struct ZGUI_MSGMAP_ENTRY
{
    ZGUI_PMSG pfn;
	String sMsgType;
	String sCtrlName;
	UINT nSig;	
};

#define ZGUI_DECLARE_MESSAGE_MAP()                                         \
private:                                                                  \
	static const ZGUI_MSGMAP_ENTRY _messageEntries[];                      \
protected:                                                                \
	static  const ZGUI_MSGMAP messageMap;				                  \
	virtual const ZGUI_MSGMAP* GetMessageMap() const;                      \


#define ZGUI_BASE_BEGIN_MESSAGE_MAP(theClass)                              \
	const ZGUI_MSGMAP* theClass::GetMessageMap() const                     \
		{ return &theClass::messageMap; }                                 \
	UILIB_COMDAT const ZGUI_MSGMAP theClass::messageMap =                  \
		{  NULL, &theClass::_messageEntries[0] };                         \
	UILIB_COMDAT const ZGUI_MSGMAP_ENTRY theClass::_messageEntries[] =     \
	{                                                                     \

#define ZGUI_BEGIN_MESSAGE_MAP(theClass, baseClass)                        \
	const ZGUI_MSGMAP* theClass::GetMessageMap() const                     \
		{ return &theClass::messageMap; }                                 \
	UILIB_COMDAT const ZGUI_MSGMAP theClass::messageMap =                  \
		{ &baseClass::messageMap, &theClass::_messageEntries[0] };        \
	UILIB_COMDAT const ZGUI_MSGMAP_ENTRY theClass::_messageEntries[] =     \
	{                                                                     \

#define ZGUI_END_MESSAGE_MAP()                                             \
	{(ZGUI_PMSG)0, L"", L"", ZGuiSig_end}                           \
};                                                                        \

#define ZGUI_ON_MSGTYPE(msgtype, memberFxn)                                \
	{(ZGUI_PMSG)&memberFxn, msgtype, L"", ZGuiSig_vn},                  \


#define ZGUI_ON_MSGTYPE_CTRNAME(msgtype,ctrname,memberFxn)                 \
	{(ZGUI_PMSG)&memberFxn, msgtype, ctrname, ZGuiSig_vn},                \


#define ZGUI_ON_CLICK_CTRNAME(ctrname,memberFxn)                           \
	{(ZGUI_PMSG)&memberFxn, ZGUI_MSGTYPE_CLICK, ctrname, ZGuiSig_vn},      \


#define ZGUI_ON_SELECTCHANGED_CTRNAME(ctrname,memberFxn)                   \
    {(ZGUI_PMSG)&memberFxn, ZGUI_MSGTYPE_SELECTCHANGED,ctrname,ZGuiSig_vn}, \


#define ZGUI_ON_KILLFOCUS_CTRNAME(ctrname,memberFxn)                       \
	{(ZGUI_PMSG)&memberFxn, ZGUI_MSGTYPE_KILLFOCUS,ctrname,ZGuiSig_vn},     \


#define ZGUI_ON_MENU_CTRNAME(ctrname,memberFxn)                            \
	{(ZGUI_PMSG)&memberFxn, ZGUI_MSGTYPE_MENU,ctrname,ZGuiSig_vn},          \

#define ZGUI_ON_TIMER()                                                    \
	{(ZGUI_PMSG)&OnTimer, ZGUI_MSGTYPE_TIMER, L"", ZGuiSig_vn},          \



#define  ZGUI_EDIT                            ("Edit")
#define  ZGUI_LIST                            ("List")
#define  ZGUI_TEXT                            ("Text")

#define  ZGUI_COMBO                           ("Combo")
#define  ZGUI_LABEL                           ("Label")
#define  ZGUI_FLASH							  ("Flash")

#define  ZGUI_BUTTON                          ("Button")
#define  ZGUI_OPTION                          ("Option")
#define  ZGUI_SLIDER                          ("Slider")

#define  ZGUI_CONTROL                         ("Control")
#define  ZGUI_ACTIVEX                         ("ActiveX")
#define  ZGUI_WEBCAM                          ("WebCam")

#define  ZGUI_LISTITEM                        ("ListItem")
#define  ZGUI_PROGRESS                        ("Progress")
#define  ZGUI_RICHEDIT                        ("RichEdit")
#define  ZGUI_CHECKBOX                        ("CheckBox")
#define  ZGUI_COMBOBOX                        ("ComboBox")
#define  ZGUI_DATETIME                        ("DateTime")
#define  ZGUI_TREEVIEW                        ("TreeView")
#define  ZGUI_TREENODE                        ("TreeNode")

#define  ZGUI_CONTAINER                       ("Container")
#define  ZGUI_TABLAYOUT                       ("TabLayout")
#define  ZGUI_SCROLLBAR                       ("ScrollBar")

#define  ZGUI_LISTHEADER                      ("ListHeader")
#define  ZGUI_TILELAYOUT                      ("TileLayout")
#define  ZGUI_WEBBROWSER                      ("WebBrowser")

#define  ZGUI_CHILDLAYOUT                     ("ChildLayout")
#define  ZGUI_LISTELEMENT                     ("ListElement")

#define  ZGUI_DIALOGLAYOUT                    ("DialogLayout")

#define  ZGUI_VERTICALLAYOUT                  ("VerticalLayout")
#define  ZGUI_LISTHEADERITEM                  ("ListHeaderItem")

#define  ZGUI_LISTTEXTELEMENT                 ("ListTextElement")

#define  ZGUI_HORIZONTALLAYOUT                ("HorizontalLayout")
#define  ZGUI_LISTLABELELEMENT                ("ListLabelElement")

#define  ZGUI_LISTCONTAINERELEMENT            ("ListContainerElement")

} // namespace zgui

