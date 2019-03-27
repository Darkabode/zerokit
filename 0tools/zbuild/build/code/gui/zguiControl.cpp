#include "zgui.h"

namespace zgui {

CControlUI::CControlUI() : 
_pManager(NULL), 
_pParent(NULL), 
m_bUpdateNeeded(true),
m_bMenuUsed(false),
m_bVisible(true), 
m_bInternVisible(true),
m_bFocused(false),
m_bEnabled(true),
m_bMouseEnabled(true),
m_bKeyboardEnabled(true),
_bFloat(false),
m_bSetPos(false),
m_chShortcut('\0'),
m_pTag(NULL),
_dwBackColor(0),
_dwBackColor2(0),
_dwBackColor3(0),
m_dwBorderColor(0),
m_dwFocusBorderColor(0),
m_bColorHSL(false),
m_nBorderSize(0)
{
    m_cXY.cx = m_cXY.cy = 0;
    _cxyFixed.cx = _cxyFixed.cy = 0;
    m_cxyMin.cx = m_cxyMin.cy = 0;
    m_cxyMax.cx = m_cxyMax.cy = 9999;
    m_cxyBorderRound.cx = m_cxyBorderRound.cy = 0;

    ::ZeroMemory(&m_rcPadding, sizeof(m_rcPadding));
    ::ZeroMemory(&_rcItem, sizeof(RECT));
    ::ZeroMemory(&_rcPaint, sizeof(RECT));
    ::ZeroMemory(&m_tRelativePos, sizeof(TRelativePosUI));
}

CControlUI::~CControlUI()
{
    if (OnDestroy) {
        OnDestroy(this);
    }

    if (_pManager != NULL) {
        _pManager->ReapObjects(this);
    }
}

String CControlUI::GetName() const
{
    return _name;
}

void CControlUI::SetName(const String& pstrName)
{
    _name = pstrName;
}

LPVOID CControlUI::GetInterface(LPCTSTR pstrName)
{
    if (lstrcmp(pstrName, DUI_CTR_CONTROL) == 0) {
        return this;
    }

    return NULL;
}

LPCTSTR CControlUI::GetClass() const
{
    return _T("ControlUI");
}

UINT CControlUI::GetControlFlags() const
{
    return 0;
}

bool CControlUI::Activate()
{
    if (!IsVisible()) {
        return false;
    }
    if (!IsEnabled()) {
        return false;
    }

    return true;
}

CPaintManagerUI* CControlUI::GetManager() const
{
    return _pManager;
}

void CControlUI::SetManager(CPaintManagerUI* pManager, CControlUI* pParent, bool bInit)
{
    _pManager = pManager;
    _pParent = pParent;
    if (bInit && _pParent) {
        Init();
    }
}

CControlUI* CControlUI::GetParent() const
{
    return _pParent;
}

String CControlUI::GetText() const
{
    return _text;
}

void CControlUI::SetText(const String& pstrText)
{
    if (_text != pstrText) {
        _text = pstrText;
        Invalidate();
    }
}

DWORD CControlUI::GetBkColor() const
{
    return _dwBackColor;
}

void CControlUI::SetBkColor(DWORD dwBackColor)
{
    if( _dwBackColor == dwBackColor ) return;

    _dwBackColor = dwBackColor;
    Invalidate();
}

DWORD CControlUI::GetBkColor2() const
{
    return _dwBackColor2;
}

void CControlUI::SetBkColor2(DWORD dwBackColor)
{
    if( _dwBackColor2 == dwBackColor ) return;

    _dwBackColor2 = dwBackColor;
    Invalidate();
}

DWORD CControlUI::GetBkColor3() const
{
    return _dwBackColor3;
}

void CControlUI::SetBkColor3(DWORD dwBackColor)
{
    if( _dwBackColor3 == dwBackColor ) return;

    _dwBackColor3 = dwBackColor;
    Invalidate();
}

const String& CControlUI::GetBkImage()
{
    return _bkImageName;
}

void CControlUI::SetBkImage(const String& imageName)
{
    if (_bkImageName != imageName) {
        _bkImageName = imageName;
        Invalidate();
    }
}

DWORD CControlUI::GetBorderColor() const
{
    return m_dwBorderColor;
}

void CControlUI::SetBorderColor(DWORD dwBorderColor)
{
    if( m_dwBorderColor == dwBorderColor ) return;

    m_dwBorderColor = dwBorderColor;
    Invalidate();
}

DWORD CControlUI::GetFocusBorderColor() const
{
    return m_dwFocusBorderColor;
}

void CControlUI::SetFocusBorderColor(DWORD dwBorderColor)
{
    if( m_dwFocusBorderColor == dwBorderColor ) return;

    m_dwFocusBorderColor = dwBorderColor;
    Invalidate();
}

bool CControlUI::IsColorHSL() const
{
    return m_bColorHSL;
}

void CControlUI::SetColorHSL(bool bColorHSL)
{
    if( m_bColorHSL == bColorHSL ) return;

    m_bColorHSL = bColorHSL;
    Invalidate();
}

int CControlUI::GetBorderSize() const
{
    return m_nBorderSize;
}

void CControlUI::SetBorderSize(int nSize)
{
    if( m_nBorderSize == nSize ) return;

    m_nBorderSize = nSize;
    Invalidate();
}

void CControlUI::SetBorderSize( RECT rc )
{
    m_rcBorderSize = rc;
    Invalidate();
}

SIZE CControlUI::GetBorderRound() const
{
    return m_cxyBorderRound;
}

void CControlUI::SetBorderRound(SIZE cxyRound)
{
    m_cxyBorderRound = cxyRound;
    Invalidate();
}

bool CControlUI::DrawImage(HDC hDC, const String& pStrImage, const String& pStrModify)
{
    return CRenderEngine::DrawImageString(hDC, _pManager, _rcItem, _rcPaint, pStrImage, pStrModify);
}

const RECT& CControlUI::GetPos() const
{
    return _rcItem;
}

void CControlUI::SetPos(RECT rc)
{
    if (rc.right < rc.left) {
        rc.right = rc.left;
    }
    if (rc.bottom < rc.top) {
        rc.bottom = rc.top;
    }

    CDuiRect invalidateRc = _rcItem;
    if (::IsRectEmpty(&invalidateRc)) {
        invalidateRc = rc;
    }

    _rcItem = rc;
    if (_pManager == NULL) {
        return;
    }

    if (!m_bSetPos) {
        m_bSetPos = true;
        if (OnSize) {
            OnSize(this);
        }
        m_bSetPos = false;
    }
    
    if (_bFloat) {
        CControlUI* pParent = GetParent();
        if (pParent != NULL) {
            RECT rcParentPos = pParent->GetPos();
            if (m_cXY.cx >= 0) {
                m_cXY.cx = _rcItem.left - rcParentPos.left;
            }
            else {
                m_cXY.cx = _rcItem.right - rcParentPos.right;
            }
            if (m_cXY.cy >= 0) {
                m_cXY.cy = _rcItem.top - rcParentPos.top;
            }
            else {
                m_cXY.cy = _rcItem.bottom - rcParentPos.bottom;
            }
            _cxyFixed.cx = _rcItem.right - _rcItem.left;
            _cxyFixed.cy = _rcItem.bottom - _rcItem.top;
        }
    }

    m_bUpdateNeeded = false;
    invalidateRc.Join(_rcItem);

    CControlUI* pParent = this;
    RECT rcTemp;
    RECT rcParent;
    while (pParent = pParent->GetParent()) {
        rcTemp = invalidateRc;
        rcParent = pParent->GetPos();
        if (!::IntersectRect(&invalidateRc, &rcTemp, &rcParent)) {
            return;
        }
    }
    _pManager->Invalidate(invalidateRc);
}

int CControlUI::GetWidth() const
{
    return _rcItem.right - _rcItem.left;
}

int CControlUI::GetHeight() const
{
    return _rcItem.bottom - _rcItem.top;
}

int CControlUI::GetX() const
{
    return _rcItem.left;
}

int CControlUI::GetY() const
{
    return _rcItem.top;
}

RECT CControlUI::GetPadding() const
{
    return m_rcPadding;
}

void CControlUI::SetPadding(RECT rcPadding)
{
    m_rcPadding = rcPadding;
    NeedParentUpdate();
}

SIZE CControlUI::GetFixedXY() const
{
    return m_cXY;
}

void CControlUI::SetFixedXY(SIZE szXY)
{
    m_cXY.cx = szXY.cx;
    m_cXY.cy = szXY.cy;
    if (!_bFloat) {
        NeedParentUpdate();
    }
    else {
        NeedUpdate();
    }
}

int CControlUI::GetFixedWidth() const
{
    return _cxyFixed.cx;
}

void CControlUI::SetFixedWidth(int cx)
{
    if (cx < 0) {
        if (_pParent != 0) {
            int parentWidth = _pParent->GetFixedWidth();
            cx *= -1;
//             if (parentWidth == 0) {
//                 parentWidth = GetManager()->GetRoot()->GetFixedWidth();
//             }
            _cxyFixed.cx = (parentWidth * cx) / 100;
        }
        else {

        }
    }
    else {
        _cxyFixed.cx = cx;
    }

    if (!_bFloat) {
        NeedParentUpdate();
    }
    else {
        NeedUpdate();
    }
}

int CControlUI::GetFixedHeight() const
{
    return _cxyFixed.cy;
}

void CControlUI::SetFixedHeight(int cy)
{
    if (cy < 0) {
        return; 
    }
    _cxyFixed.cy = cy;
    if (!_bFloat) {
        NeedParentUpdate();
    }
    else {
        NeedUpdate();
    }
}

int CControlUI::GetMinWidth() const
{
    return m_cxyMin.cx;
}

void CControlUI::SetMinWidth(int cx)
{
    if (m_cxyMin.cx == cx) {
        return;
    }

    if (cx < 0) {
        return;
    }
    m_cxyMin.cx = cx;
    if (!_bFloat) {
        NeedParentUpdate();
    }
    else {
        NeedUpdate();
    }
}

int CControlUI::GetMaxWidth() const
{
    return m_cxyMax.cx;
}

void CControlUI::SetMaxWidth(int cx)
{
    if (m_cxyMax.cx == cx) {
        return;
    }

    if (cx < 0) {
        return;
    }
    m_cxyMax.cx = cx;
    if (!_bFloat) {
        NeedParentUpdate();
    }
    else {
        NeedUpdate();
    }
}

int CControlUI::GetMinHeight() const
{
    return m_cxyMin.cy;
}

void CControlUI::SetMinHeight(int cy)
{
    if (m_cxyMin.cy == cy) {
        return;
    }

    if (cy < 0) {
        return;
    }
    m_cxyMin.cy = cy;
    if (!_bFloat) {
        NeedParentUpdate();
    }
    else {
        NeedUpdate();
    }
}

int CControlUI::GetMaxHeight() const
{
    return m_cxyMax.cy;
}

void CControlUI::SetMaxHeight(int cy)
{
    if (m_cxyMax.cy == cy) {
        return;
    }

    if (cy < 0) {
        return; 
    }
    m_cxyMax.cy = cy;
    if (!_bFloat) {
        NeedParentUpdate();
    }
    else {
        NeedUpdate();
    }
}

void CControlUI::SetRelativePos(SIZE szMove,SIZE szZoom)
{
    m_tRelativePos.bRelative = TRUE;
    m_tRelativePos.nMoveXPercent = szMove.cx;
    m_tRelativePos.nMoveYPercent = szMove.cy;
    m_tRelativePos.nZoomXPercent = szZoom.cx;
    m_tRelativePos.nZoomYPercent = szZoom.cy;
}

void CControlUI::SetRelativeParentSize(SIZE sz)
{
    m_tRelativePos.szParent = sz;
}

TRelativePosUI CControlUI::GetRelativePos() const
{
    return m_tRelativePos;
}

bool CControlUI::IsRelativePos() const
{
    return m_tRelativePos.bRelative;
}

String CControlUI::GetToolTip() const
{
    return _tooltipText;
}

void CControlUI::SetToolTip(const String& pstrText)
{
    _tooltipText = pstrText;
}


TCHAR CControlUI::GetShortcut() const
{
    return m_chShortcut;
}

void CControlUI::SetShortcut(TCHAR ch)
{
    m_chShortcut = ch;
}

bool CControlUI::IsContextMenuUsed() const
{
    return m_bMenuUsed;
}

void CControlUI::SetContextMenuUsed(bool bMenuUsed)
{
    m_bMenuUsed = bMenuUsed;
}

const String& CControlUI::GetUserData()
{
    return _userData;
}

void CControlUI::SetUserData(const String& pstrText)
{
    _userData = pstrText;
}

UINT_PTR CControlUI::GetTag() const
{
    return m_pTag;
}

void CControlUI::SetTag(UINT_PTR pTag)
{
    m_pTag = pTag;
}

bool CControlUI::IsVisible() const
{

    return m_bVisible && m_bInternVisible;
}

void CControlUI::SetVisible(bool bVisible)
{
    if( m_bVisible == bVisible ) return;

    bool v = IsVisible();
    m_bVisible = bVisible;
    if (m_bFocused) {
        m_bFocused = false;
    }
	if (!bVisible && _pManager && _pManager->GetFocus() == this) {
		_pManager->SetFocus(NULL) ;
	}
    if (IsVisible() != v) {
        NeedParentUpdate();
    }
}

void CControlUI::SetInternVisible(bool bVisible)
{
    m_bInternVisible = bVisible;
	if (!bVisible && _pManager && _pManager->GetFocus() == this) {
		_pManager->SetFocus(NULL) ;
	}
}

bool CControlUI::IsEnabled() const
{
    return m_bEnabled;
}

void CControlUI::SetEnabled(bool bEnabled)
{
    if( m_bEnabled == bEnabled ) return;

    m_bEnabled = bEnabled;
    Invalidate();
}

bool CControlUI::IsMouseEnabled() const
{
    return m_bMouseEnabled;
}

void CControlUI::SetMouseEnabled(bool bEnabled)
{
    m_bMouseEnabled = bEnabled;
}

bool CControlUI::IsKeyboardEnabled() const
{
	return m_bKeyboardEnabled ;
}
void CControlUI::SetKeyboardEnabled(bool bEnabled)
{
	m_bKeyboardEnabled = bEnabled ; 
}

bool CControlUI::IsFocused() const
{
    return m_bFocused;
}

void CControlUI::SetFocus()
{
    if( _pManager != NULL ) _pManager->SetFocus(this);
}

bool CControlUI::IsFloat() const
{
    return _bFloat;
}

void CControlUI::SetFloat(bool bFloat)
{
    if (_bFloat != bFloat) {
        _bFloat = bFloat;
        NeedParentUpdate();
    }
}

CControlUI* CControlUI::FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags)
{
    if ((uFlags & UIFIND_VISIBLE) != 0 && !IsVisible()) {
        return NULL;
    }
    if ((uFlags & UIFIND_ENABLED) != 0 && !IsEnabled()) {
        return NULL;
    }
    if ((uFlags & UIFIND_HITTEST) != 0 && (!m_bMouseEnabled || !::PtInRect(&_rcItem, * static_cast<LPPOINT>(pData)))) {
        return NULL;
    }

    return Proc(this, pData);
}

void CControlUI::Invalidate()
{
    if (IsVisible()) {
        RECT invalidateRc = _rcItem;

        CControlUI* pParent = this;
        RECT rcTemp;
        RECT rcParent;
        while (pParent = pParent->GetParent()) {
            rcTemp = invalidateRc;
            rcParent = pParent->GetPos();
            if (!::IntersectRect(&invalidateRc, &rcTemp, &rcParent)) {
                return;
            }
        }

        if (_pManager != NULL) {
            _pManager->Invalidate(invalidateRc);
        }
    }
}

bool CControlUI::IsUpdateNeeded() const
{
    return m_bUpdateNeeded;
}

void CControlUI::NeedUpdate()
{
    if (IsVisible()) {
        m_bUpdateNeeded = true;
        Invalidate();

        if (_pManager != NULL) {
            _pManager->NeedUpdate();
        }
    }
}

void CControlUI::NeedParentUpdate()
{
    if (GetParent()) {
        GetParent()->NeedUpdate();
        GetParent()->Invalidate();
    }
    else {
        NeedUpdate();
    }

    if (_pManager != NULL) {
        _pManager->NeedUpdate();
    }
}

DWORD CControlUI::GetAdjustColor(DWORD dwColor)
{
    if( !m_bColorHSL ) return dwColor;
    short H, S, L;
    CPaintManagerUI::GetHSL(&H, &S, &L);
    return CRenderEngine::AdjustColor(dwColor, H, S, L);
}

void CControlUI::Init()
{
    DoInit();
    if( OnInit ) OnInit(this);
}

void CControlUI::DoInit()
{

}

void CControlUI::Event(TEventUI& event)
{
    if( OnEvent(&event) ) DoEvent(event);
}

void CControlUI::DoEvent(TEventUI& event)
{
    if( event.Type == UIEVENT_SETCURSOR )
    {
        ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
        return;
    }
    if( event.Type == UIEVENT_SETFOCUS ) 
    {
        m_bFocused = true;
        Invalidate();
        return;
    }
    if( event.Type == UIEVENT_KILLFOCUS ) 
    {
        m_bFocused = false;
        Invalidate();
        return;
    }
    if( event.Type == UIEVENT_TIMER )
    {
        _pManager->SendNotify(this, DUI_MSGTYPE_TIMER, event.wParam, event.lParam);
        return;
    }
    if( event.Type == UIEVENT_CONTEXTMENU )
    {
        if( IsContextMenuUsed() ) {
            _pManager->SendNotify(this, DUI_MSGTYPE_MENU, event.wParam, event.lParam);
            return;
        }
    }
    if (_pParent != NULL) {
        _pParent->DoEvent(event);
    }
}

void CControlUI::SetVirtualWnd(const String& pstrValue)
{
    m_sVirtualWnd = pstrValue;
    _pManager->UsedVirtualWnd(true);
}

String CControlUI::GetVirtualWnd() const
{
    String str;
    if (!m_sVirtualWnd.isEmpty()) {
        str = m_sVirtualWnd;
    }
    else {
        CControlUI* pParent = GetParent();
        if (pParent != NULL) {
            str = pParent->GetVirtualWnd();
        }
        else {
            str = _T("");
        }
    }
    return str;
}

void CControlUI::SetAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "pos") {
        RECT rcPos = { 0 };
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcPos.left, (int&)rcPos.top, (int&)rcPos.right, (int&)rcPos.bottom)) {
            SIZE szXY = {rcPos.left >= 0 ? rcPos.left : rcPos.right, rcPos.top >= 0 ? rcPos.top : rcPos.bottom};
            SetFixedXY(szXY);
            SetFixedWidth(rcPos.right - rcPos.left);
            SetFixedHeight(rcPos.bottom - rcPos.top);
        }
    }
    else if (pstrName == "relativepos") {
        SIZE szMove,szZoom;
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)szMove.cx, (int&)szMove.cy, (int&)szZoom.cx, (int&)szZoom.cy)) {
            SetRelativePos(szMove,szZoom);
        }
    }
    else if (pstrName == "padding") {
        RECT rcPadding = { 0 };
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcPadding.left, (int&)rcPadding.top, (int&)rcPadding.right, (int&)rcPadding.bottom)) {
            SetPadding(rcPadding);
        }
    }
    else if (pstrName == "bkcolor" || pstrName == "bkcolor1") {
        SetBkColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "bkcolor2") {
        SetBkColor2((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "bkcolor3") {
        SetBkColor3((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "gradient") {
        StringArray vals;
        vals.addTokens(pstrValue, ":", String::empty);
        if (vals.size() == 2) {
            _gradientVertical = (vals[0].getIntValue() != 0);
            _gradientSteps = vals[1].getIntValue();
        }
    }
    else if (pstrName == "bordercolor") {
        SetBorderColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "focusbordercolor") {
        SetFocusBorderColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "colorhsl") {
        SetColorHSL(pstrValue == "true");
    }
    else if (pstrName == "bordersize") {
        String nValue = pstrValue;
        if(pstrValue.indexOf(",") == -1)
            SetBorderSize(pstrValue.getIntValue());
        else {
            RECT rcPadding = { 0 };
            if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcPadding.left, (int&)rcPadding.top, (int&)rcPadding.right, (int&)rcPadding.bottom)) {
                SetBorderSize(rcPadding);
            }
        }
    }
    else if (pstrName == "leftbordersize") SetLeftBorderSize(pstrValue.getIntValue());
    else if (pstrName == "topbordersize") SetTopBorderSize(pstrValue.getIntValue());
    else if (pstrName == "rightbordersize") SetRightBorderSize(pstrValue.getIntValue());
    else if (pstrName == "bottombordersize") SetBottomBorderSize(pstrValue.getIntValue());
    else if (pstrName == "borderstyle") SetBorderStyle(pstrValue.getIntValue());
    else if (pstrName == "borderround") {
        SIZE cxyRound = { 0 };
        if (Helper::splitString(pstrValue, ",", String::empty, (int&)cxyRound.cx, (int&)cxyRound.cy)) {
            SetBorderRound(cxyRound);
        }
    }
    else if (pstrName == "bkimage") {
        SetBkImage(pstrValue.trim());
    }
    else if (pstrName == "width") {
        SetFixedWidth(pstrValue.getIntValue());
    }
    else if (pstrName == "height") {
        SetFixedHeight(pstrValue.getIntValue());
    }
    else if (pstrName == "minwidth") {
        SetMinWidth(pstrValue.getIntValue());
    }
    else if (pstrName == "minheight") {
        SetMinHeight(pstrValue.getIntValue());
    }
    else if (pstrName == "maxwidth") {
        SetMaxWidth(pstrValue.getIntValue());
    }
    else if (pstrName == "maxheight") {
        SetMaxHeight(pstrValue.getIntValue());
    }
    else if (pstrName == "name") {
        SetName(pstrValue.trim());
    }
    else if (pstrName == "text") {
        SetText(pstrValue.trim());
    }
    else if (pstrName == "tooltip") {
        SetToolTip(pstrValue.trim());
    }
    else if (pstrName == "userdata") {
        SetUserData(pstrValue.trim());
    }
    else if (pstrName == "enabled") {
        SetEnabled(pstrValue == "true");
    }
    else if (pstrName == "mouse") {
        SetMouseEnabled(pstrValue == "true");
    }
    else if (pstrName == "keyboard") {
        SetKeyboardEnabled(pstrValue == "true");
    }
    else if (pstrName == "visible") {
        SetVisible(pstrValue == "true");
    }
    else if (pstrName == "float") {
        SetFloat(pstrValue == "true");
    }
    else if (pstrName == "shortcut") {
        SetShortcut(pstrValue[0]);
    }
    else if (pstrName == "menu") {
        SetContextMenuUsed(pstrValue == "true");
    }
}

CControlUI* CControlUI::ApplyAttributeList(const String& attrList)
{
    LPCTSTR pstrList = attrList.toWideCharPointer();
    String sItem;
    String sValue;

    while (*pstrList != _T('\0')) {
        sItem = String::empty;
        sValue = String::empty;
        while (*pstrList != _T('\0') && *pstrList != _T('=')) {
            LPTSTR pstrTemp = ::CharNext(pstrList);
            while (pstrList < pstrTemp) {
                sItem += *pstrList++;
            }
        }
        ASSERT(*pstrList == _T('='));
        if (*pstrList++ != _T('=')) {
            return this;
        }

        ASSERT( *pstrList == _T('\"') );
        if (*pstrList++ != _T('\"')) {
            return this;
        }

        while (*pstrList != _T('\0') && *pstrList != _T('\"')) {
            LPTSTR pstrTemp = ::CharNext(pstrList);
            while (pstrList < pstrTemp) {
                sValue += *pstrList++;
            }
        }
        ASSERT(*pstrList == _T('\"'));
        if (*pstrList++ != _T('\"')) {
            return this;
        }

        sItem = sItem.trim();
        sValue = sValue.trim();
        SetAttribute(sItem, sValue);
        if (*pstrList++ != _T(' ')) {
            return this;
        }
    }
    return this;
}

SIZE CControlUI::EstimateSize(SIZE szAvailable)
{
    return _cxyFixed;
}

void CControlUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if (!::IntersectRect(&_rcPaint, &rcPaint, &_rcItem)) {
        return;
    }

    if (m_cxyBorderRound.cx > 0 || m_cxyBorderRound.cy > 0) {
        CRenderClip roundClip;
        CRenderClip::GenerateRoundClip(hDC, _rcPaint,  _rcItem, m_cxyBorderRound.cx, m_cxyBorderRound.cy, roundClip);
        PaintBkColor(hDC);
        PaintBkImage(hDC);
        PaintStatusImage(hDC);
        PaintText(hDC);
        PaintBorder(hDC);
    }
    else {
        PaintBkColor(hDC);
        PaintBkImage(hDC);
        PaintStatusImage(hDC);
        PaintText(hDC); 
        PaintBorder(hDC);
    } 
}

void CControlUI::PaintBkColor(HDC hDC)
{
    if (_dwBackColor != 0) {
        if (_dwBackColor2 != 0) {
            if (_dwBackColor3 != 0) {
                RECT rc = _rcItem; 

                if (_gradientVertical) {
                    rc.bottom = (rc.bottom + rc.top) / 2; 
                    CRenderEngine::DrawGradient(hDC, rc, GetAdjustColor(_dwBackColor), GetAdjustColor(_dwBackColor2), _gradientVertical, _gradientSteps);
                    rc.top = rc.bottom;
                    rc.bottom = _rcItem.bottom;
                    CRenderEngine::DrawGradient(hDC, rc, GetAdjustColor(_dwBackColor2), GetAdjustColor(_dwBackColor3), _gradientVertical, _gradientSteps);
                }
                else {
                    rc.right = (rc.right + rc.left) / 2; 
                    CRenderEngine::DrawGradient(hDC, rc, GetAdjustColor(_dwBackColor), GetAdjustColor(_dwBackColor2), _gradientVertical, _gradientSteps);
                    rc.left = rc.right;
                    rc.right = _rcItem.right;
                    CRenderEngine::DrawGradient(hDC, rc, GetAdjustColor(_dwBackColor2), GetAdjustColor(_dwBackColor3), _gradientVertical, _gradientSteps);
                }
            }
            else {
                CRenderEngine::DrawGradient(hDC, _rcItem, GetAdjustColor(_dwBackColor), GetAdjustColor(_dwBackColor2), _gradientVertical, _gradientSteps);
            }
        }
        else if (_dwBackColor >= 0xFF000000) {
            CRenderEngine::DrawColor(hDC, _rcPaint, GetAdjustColor(_dwBackColor));
        }
        else {
            CRenderEngine::DrawColor(hDC, _rcItem, GetAdjustColor(_dwBackColor));
        }
    }
}

void CControlUI::PaintBkImage(HDC hDC)
{
    if (_bkImageName.isEmpty()) {
        return;
    }
    if (!DrawImage(hDC, _bkImageName)) {
        _bkImageName = String::empty;
    }
}

void CControlUI::PaintStatusImage(HDC hDC)
{
    return;
}

void CControlUI::PaintText(HDC hDC)
{
    return;
}

void CControlUI::PaintBorder(HDC hDC)
{
    if (m_dwBorderColor != 0 || m_dwFocusBorderColor != 0) {
        if (m_nBorderSize > 0 && ( m_cxyBorderRound.cx > 0 || m_cxyBorderRound.cy > 0 )) { //»­Ô²½Ç±ß¿ò
            if (IsFocused() && m_dwFocusBorderColor != 0) {
				CRenderEngine::DrawRoundRect(hDC, _rcItem, m_nBorderSize, m_cxyBorderRound.cx, m_cxyBorderRound.cy, GetAdjustColor(m_dwFocusBorderColor));
            }
            else {
				CRenderEngine::DrawRoundRect(hDC, _rcItem, m_nBorderSize, m_cxyBorderRound.cx, m_cxyBorderRound.cy, GetAdjustColor(m_dwBorderColor));
            }
		}
		else {
            if (IsFocused() && m_dwFocusBorderColor != 0 && m_nBorderSize > 0) {
				CRenderEngine::DrawRect(hDC, _rcItem, m_nBorderSize, GetAdjustColor(m_dwFocusBorderColor));
            }
            else if(m_rcBorderSize.left > 0 || m_rcBorderSize.top > 0 || m_rcBorderSize.right > 0 || m_rcBorderSize.bottom > 0) {
                RECT rcBorder;

                if(m_rcBorderSize.left > 0){
                    rcBorder		= _rcItem;
                    rcBorder.right	= _rcItem.left;
                    CRenderEngine::DrawLine(hDC,rcBorder,m_rcBorderSize.left,GetAdjustColor(m_dwBorderColor),m_nBorderStyle);
                }
                if(m_rcBorderSize.top > 0){
                    rcBorder		= _rcItem;
                    rcBorder.bottom	= _rcItem.top;
                    CRenderEngine::DrawLine(hDC,rcBorder,m_rcBorderSize.top,GetAdjustColor(m_dwBorderColor),m_nBorderStyle);
                }
                if(m_rcBorderSize.right > 0){
                    rcBorder		= _rcItem;
                    rcBorder.left	= _rcItem.right;
                    CRenderEngine::DrawLine(hDC,rcBorder,m_rcBorderSize.right,GetAdjustColor(m_dwBorderColor),m_nBorderStyle);
                }
                if(m_rcBorderSize.bottom > 0){
                    rcBorder		= _rcItem;
                    rcBorder.top	= _rcItem.bottom;
                    CRenderEngine::DrawLine(hDC,rcBorder,m_rcBorderSize.bottom,GetAdjustColor(m_dwBorderColor),m_nBorderStyle);
                }
            }
            else if(m_nBorderSize > 0) {
				CRenderEngine::DrawRect(hDC, _rcItem, m_nBorderSize, GetAdjustColor(m_dwBorderColor));
            }
		}
	}
}

void CControlUI::DoPostPaint(HDC hDC, const RECT& rcPaint)
{
    return;
}

int CControlUI::GetLeftBorderSize() const
{
	return m_rcBorderSize.left;
}

void CControlUI::SetLeftBorderSize( int nSize )
{
	m_rcBorderSize.left = nSize;
	Invalidate();
}

int CControlUI::GetTopBorderSize() const
{
	return m_rcBorderSize.top;
}

void CControlUI::SetTopBorderSize( int nSize )
{
	m_rcBorderSize.top = nSize;
	Invalidate();
}

int CControlUI::GetRightBorderSize() const
{
	return m_rcBorderSize.right;
}

void CControlUI::SetRightBorderSize( int nSize )
{
	m_rcBorderSize.right = nSize;
	Invalidate();
}

int CControlUI::GetBottomBorderSize() const
{
	return m_rcBorderSize.bottom;
}

void CControlUI::SetBottomBorderSize( int nSize )
{
	m_rcBorderSize.bottom = nSize;
	Invalidate();
}

int CControlUI::GetBorderStyle() const
{
	return m_nBorderStyle;
}

void CControlUI::SetBorderStyle( int nStyle )
{
	m_nBorderStyle = nStyle;
	Invalidate();
}

} // namespace zgui
