#include "zgui.h"

namespace zgui {

const String Control::CLASS_NAME = ZGUI_CONTROL;

Control::Control() : 
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
_dwBorderColor(0),
_dwFocusBorderColor(0),
m_bColorHSL(false),
m_nBorderSize(0),
m_nBorderStyle(PS_SOLID),
m_nTooltipWidth(300)
{
#ifdef ZGUI_USE_ANIMATION

	_anim.pOwner = this;

	_curEffects._iZoom = -1;
	_curEffects._dFillingBK = 0xffffffff;
	_curEffects._iOffectX = 0;
	_curEffects._iOffectY = 0;
	_curEffects._iAlpha = -255;
	_curEffects._fRotation = 0.0;
	_curEffects._iNeedTimer = 350;
#endif ZGUI_USE_ANIMATION

    m_cXY.cx = m_cXY.cy = 0;
    _cxyFixed.cx = _cxyFixed.cy = 0;
    m_cxyMin.cx = m_cxyMin.cy = 0;
    m_cxyMax.cx = m_cxyMax.cy = 9999;
    m_cxyBorderRound.cx = m_cxyBorderRound.cy = 0;

    __stosb((uint8_t*)&_rcPadding, 0, sizeof(_rcPadding));
    __stosb((uint8_t*)&_rcItem, 0, sizeof(_rcItem));
    __stosb((uint8_t*)&_rcPaint, 0, sizeof(_rcPaint));
    __stosb((uint8_t*)&m_tRelativePos, 0, sizeof(TRelativePosUI));
    __stosb((uint8_t*)&m_rcBorderSize, 0, sizeof(m_rcBorderSize));
    
}

Control::~Control()
{
    if (OnDestroy) {
        OnDestroy(this);
    }

    if (_pManager != NULL) {
        _pManager->ReapObjects(this);
    }
}

String Control::GetName() const
{
    return _name;
}

void Control::SetName(const String& name)
{
    _name = name;
}

LPVOID Control::getInterface(const String& name)
{
    if (name == ZGUI_CONTROL) {
        return this;
    }

    return NULL;
}

const String& Control::getClass() const
{
    return CLASS_NAME;
}

UINT Control::GetControlFlags() const
{
    return 0;
}

bool Control::Activate()
{
    if (!IsVisible()) {
        return false;
    }
    if (!IsEnabled()) {
        return false;
    }

    return true;
}

void Control::SetManager(PaintManager* pManager, Control* pParent, bool bInit)
{
    _pManager = pManager;
    _pParent = pParent;
    if (bInit && _pParent) {
        Init();
    }
}

Control* Control::GetParent() const
{
    return _pParent;
}

String Control::getText() const
{
    return _text;
}

void Control::setText(const String& newText, const bool needTranslate)
{
	String transText;
	// Translate before update
	if (needTranslate) {
		_unifiedText = newText;
		transText = Language::getInstance()->scanTextAndTranslate(newText);
	}
	else {
		transText = newText;
	}
	

	if (_text != transText) {
		_text = transText;
        Invalidate();
    }
}

void Control::updateText()
{
	if (!_unifiedText.isEmpty()) {
		_text = Language::getInstance()->scanTextAndTranslate(_unifiedText);
		Invalidate();
	}
}

uint32_t Control::GetBkColor() const
{
    return _dwBackColor;
}

void Control::SetBkColor(uint32_t dwBackColor)
{
	if (_dwBackColor != dwBackColor) {
		_dwBackColor = dwBackColor;
		Invalidate();
	}    
}

uint32_t Control::GetBkColor2() const
{
    return _dwBackColor2;
}

void Control::SetBkColor2(uint32_t dwBackColor)
{
	if (_dwBackColor2 != dwBackColor) {
		_dwBackColor2 = dwBackColor;
		Invalidate();
	}
}

uint32_t Control::GetBkColor3() const
{
    return _dwBackColor3;
}

void Control::SetBkColor3(uint32_t dwBackColor)
{
	if (_dwBackColor3 != dwBackColor) {
		_dwBackColor3 = dwBackColor;
		Invalidate();
	}
}

const String& Control::GetBkImage()
{
    return _bkImageName;
}

void Control::SetBkImage(const String& imageName)
{
    if (_bkImageName != imageName) {
        _bkImageName = imageName;
        Invalidate();
    }
}

uint32_t Control::GetBorderColor() const
{
    return _dwBorderColor;
}

void Control::SetBorderColor(uint32_t dwBorderColor)
{
    if (_dwBorderColor != dwBorderColor) {
		_dwBorderColor = dwBorderColor;
		Invalidate();
    }
}

uint32_t Control::GetDisabledBorderColor() const
{
	return _dwDisabledBorderColor;
}

void Control::SetDisabledBorderColor(uint32_t dwBorderColor)
{
	if (_dwDisabledBorderColor != dwBorderColor) {
		_dwDisabledBorderColor = dwBorderColor;
		Invalidate();
	}
}

uint32_t Control::GetFocusBorderColor() const
{
    return _dwFocusBorderColor;
}

void Control::SetFocusBorderColor(uint32_t dwBorderColor)
{
	if (_dwFocusBorderColor != dwBorderColor) {
		_dwFocusBorderColor = dwBorderColor;
		Invalidate();
	}
}

bool Control::IsColorHSL() const
{
    return m_bColorHSL;
}

void Control::SetColorHSL(bool bColorHSL)
{
	if (m_bColorHSL != bColorHSL) {
		m_bColorHSL = bColorHSL;
		Invalidate();
	}
}

int Control::GetBorderSize() const
{
    return m_nBorderSize;
}

void Control::SetBorderSize(int nSize)
{
	if (m_nBorderSize != nSize) {
		m_nBorderSize = nSize;
		Invalidate();
	}
}

void Control::SetBorderSize(RECT rc)
{
    m_rcBorderSize = rc;
    Invalidate();
}

SIZE Control::GetBorderRound() const
{
    return m_cxyBorderRound;
}

void Control::SetBorderRound(SIZE cxyRound)
{
    m_cxyBorderRound = cxyRound;
    Invalidate();
}

bool Control::DrawImage(HDC hDC, const String& strImage, const String& strModify)
{
    return RenderEngine::drawImageString(hDC, _pManager, _rcItem, _rcPaint, strImage, strModify);
}

const RECT& Control::GetPos() const
{
    return _rcItem;
}

void Control::SetPos(RECT rc)
{
    if (rc.right < rc.left) {
        rc.right = rc.left;
    }
    if (rc.bottom < rc.top) {
        rc.bottom = rc.top;
    }

    Rect invalidateRc = _rcItem;
    if (::fn_IsRectEmpty(&invalidateRc)) {
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
        Control* pParent = GetParent();
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
    invalidateRc.joinWith(_rcItem);

    Control* pParent = this;
    RECT rcTemp;
    RECT rcParent;
    while (pParent = pParent->GetParent()) {
        rcTemp = invalidateRc;
        rcParent = pParent->GetPos();
        if (!fn_IntersectRect(&invalidateRc, &rcTemp, &rcParent)) {
            return;
        }
    }
    _pManager->Invalidate(invalidateRc);
}

int Control::GetWidth() const
{
    return _rcItem.right - _rcItem.left;
}

int Control::GetHeight() const
{
    return _rcItem.bottom - _rcItem.top;
}

int Control::GetX() const
{
    return _rcItem.left;
}

int Control::GetY() const
{
    return _rcItem.top;
}

RECT Control::GetPadding() const
{
    return _rcPadding;
}

void Control::SetPadding(RECT rcPadding)
{
    _rcPadding = rcPadding;
    NeedParentUpdate();
}

SIZE Control::GetFixedXY() const
{
    return m_cXY;
}

void Control::SetFixedXY(SIZE szXY)
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

int Control::GetFixedWidth() const
{
    return _cxyFixed.cx;
}

void Control::SetFixedWidth(int cx)
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

int Control::GetFixedHeight() const
{
    return _cxyFixed.cy;
}

void Control::SetFixedHeight(int cy)
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

int Control::GetMinWidth() const
{
    return m_cxyMin.cx;
}

void Control::SetMinWidth(int cx)
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

int Control::GetMaxWidth() const
{
    return m_cxyMax.cx;
}

void Control::SetMaxWidth(int cx)
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

int Control::GetMinHeight() const
{
    return m_cxyMin.cy;
}

void Control::SetMinHeight(int cy)
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

int Control::GetMaxHeight() const
{
    return m_cxyMax.cy;
}

void Control::SetMaxHeight(int cy)
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

void Control::SetRelativePos(SIZE szMove,SIZE szZoom)
{
    m_tRelativePos.bRelative = TRUE;
    m_tRelativePos.nMoveXPercent = szMove.cx;
    m_tRelativePos.nMoveYPercent = szMove.cy;
    m_tRelativePos.nZoomXPercent = szZoom.cx;
    m_tRelativePos.nZoomYPercent = szZoom.cy;
}

void Control::SetRelativeParentSize(SIZE sz)
{
    m_tRelativePos.szParent = sz;
}

TRelativePosUI Control::GetRelativePos() const
{
    return m_tRelativePos;
}

bool Control::IsRelativePos() const
{
    return m_tRelativePos.bRelative;
}

const String& Control::GetToolTip() const
{
	if (_tooltipText.isEmpty()) {
		Control* pParent = GetParent();
		if (pParent != 0) {
			POINT ptMouse = _pManager->GetMousePos();
			if (fn_PtInRect(&pParent->GetPos(), ptMouse)) {
				return pParent->GetToolTip();
			}
		}
	}
	return _tooltipText;
}

void Control::SetToolTip(const String& pstrText)
{
	String transText = Language::getInstance()->scanTextAndTranslate(pstrText.replace("<n>", "\r\n", true));

	if (_tooltipText != transText) {
		_tooltipText = transText;
	}
}

void Control::SetToolTipWidth(int nWidth)
{
	m_nTooltipWidth = nWidth;
}

int Control::GetToolTipWidth()
{
	return m_nTooltipWidth;
}

TCHAR Control::GetShortcut() const
{
    return m_chShortcut;
}

void Control::SetShortcut(TCHAR ch)
{
    m_chShortcut = ch;
}

bool Control::IsContextMenuUsed() const
{
    return m_bMenuUsed;
}

void Control::SetContextMenuUsed(bool bMenuUsed)
{
    m_bMenuUsed = bMenuUsed;
}

const String& Control::GetUserData()
{
    return _userData;
}

void Control::SetUserData(const String& pstrText)
{
    _userData = pstrText;
}

UINT_PTR Control::GetTag() const
{
    return m_pTag;
}

void Control::SetTag(UINT_PTR pTag)
{
    m_pTag = pTag;
}

#ifdef ZGUI_USE_ANIMATION

void Control::setEffectsStyle(const String& effectStyle, TEffectAge* pTEffectAge)
{
	String sItem;
	String sValue;
	String sAnim;
	StringArray params;

	TEffectAge* pcTEffectAge = pTEffectAge ? pTEffectAge : &_curEffects;

	params.addTokens(effectStyle, " \t", "'");
	for (int i = 0; i < params.size(); ++i) {
		StringArray param;
		param.addTokens(params[i], "=", "'");
		if (param.size() == 2) {
			sItem = param[0].trim();
			sValue = param[1].trim().unquoted().trim();

			if (!sValue.isEmpty()) {
				if (sItem == "anim") {
					sAnim = sValue;

					if (sValue == "zoom+") {
						if (pcTEffectAge->_iZoom > 0) {
							pcTEffectAge->_iZoom = (pcTEffectAge->_iZoom - pcTEffectAge->_iZoom * 2);
						}
						if (pcTEffectAge->_iZoom == 0) {
							pcTEffectAge->_iZoom = -1;
						}
						pcTEffectAge->_iAlpha = -255;
						pcTEffectAge->_fRotation = 0.0;
					}
					else if (sValue == "zoom-") {
						if (pcTEffectAge->_iZoom < 0) {
							pcTEffectAge->_iZoom = (pcTEffectAge->_iZoom - pcTEffectAge->_iZoom * 2);
						}
						if (pcTEffectAge->_iZoom == 0) {
							pcTEffectAge->_iZoom = 1;
						}
						pcTEffectAge->_iAlpha = 255;
						pcTEffectAge->_fRotation = 0.0;
					}
					else if (sValue == "left2right") {
						if (pcTEffectAge->_iOffectX > 0) {
							pcTEffectAge->_iOffectX = (pcTEffectAge->_iOffectX - pcTEffectAge->_iOffectX * 2);
						}
						pcTEffectAge->_iAlpha = 255;
						pcTEffectAge->_iZoom = 0;
						pcTEffectAge->_iOffectY = 0;
						pcTEffectAge->_fRotation = 0.0;
					}
					else if (sValue == "right2left") {
						if (pcTEffectAge->_iOffectX < 0) {
							pcTEffectAge->_iOffectX = (pcTEffectAge->_iOffectX - pcTEffectAge->_iOffectX * 2);
						}
						pcTEffectAge->_iAlpha = 255;
						pcTEffectAge->_iZoom = 0;
						pcTEffectAge->_iOffectY = 0;
						pcTEffectAge->_fRotation = 0.0;
					}
					else if (sValue == "top2bottom") {
						if (pcTEffectAge->_iOffectY > 0) {
							pcTEffectAge->_iOffectY = (pcTEffectAge->_iOffectY - pcTEffectAge->_iOffectY * 2);
						}
						pcTEffectAge->_iAlpha = 255;
						pcTEffectAge->_iZoom = 0;
						pcTEffectAge->_iOffectX = 0;
						pcTEffectAge->_fRotation = 0.0;
					}
					else if (sValue == "bottom2top") {
						if (pcTEffectAge->_iOffectY < 0) {
							pcTEffectAge->_iOffectY = (pcTEffectAge->_iOffectY - pcTEffectAge->_iOffectY * 2);
						}
						pcTEffectAge->_iAlpha = 255;
						pcTEffectAge->_iZoom = 0;
						pcTEffectAge->_iOffectX = 0;
						pcTEffectAge->_fRotation = 0.0;
					}
				}
				else if (sItem == "offset") {
					if (sAnim == "zoom+" || sAnim == "zoom-") {
						pcTEffectAge->_iOffectX = 0;
						pcTEffectAge->_iOffectY = 0;
					}
					else if (sAnim == "left2right" || sAnim == "right2left") {
						pcTEffectAge->_iOffectX = sValue.getIntValue();
						pcTEffectAge->_iOffectY = 0;

						if (sAnim == "left2right") {
							if (pcTEffectAge->_iOffectX > 0) {
								pcTEffectAge->_iOffectX = (pcTEffectAge->_iOffectX - pcTEffectAge->_iOffectX * 2);
							}
						}
						else if (sAnim == "right2left") {
							if (pcTEffectAge->_iOffectX < 0) {
								pcTEffectAge->_iOffectX = (pcTEffectAge->_iOffectX - pcTEffectAge->_iOffectX * 2);
							}
						}
					}
					else if (sAnim == "top2bottom" || sAnim == "bottom2top") {
						pcTEffectAge->_iOffectX = 0;
						pcTEffectAge->_iOffectY = sValue.getIntValue();

						if (sAnim == "top2bottom") {
							if (pcTEffectAge->_iOffectY > 0) {
								pcTEffectAge->_iOffectY = (pcTEffectAge->_iOffectY - pcTEffectAge->_iOffectY * 2);
							}
						}
						if (sAnim == "bottom2top") {
							if (pcTEffectAge->_iOffectY < 0) {
								pcTEffectAge->_iOffectY = (pcTEffectAge->_iOffectY - pcTEffectAge->_iOffectY * 2);
							}
						}
					}
				}
				else if (sItem == "needtimer") {
					pcTEffectAge->_iNeedTimer = sValue.getIntValue();
				}
				else if (sItem == "fillingbk") {
					if (sValue == "none") {
						sValue == "ffffffff";
					}

					pcTEffectAge->_dFillingBK = (uint32_t)sValue.getHexValue32();
				}
				else if (sItem == "zoom") {
					pcTEffectAge->_iZoom = sValue.getIntValue();
				}
				else if (sItem == "fillingbk") {
					if (sValue == "none") {
						sValue == "ffffffff";
					}

					pcTEffectAge->_dFillingBK = (uint32_t)sValue.getHexValue32();
				}
				else if (sItem == "offsetx") {
					pcTEffectAge->_iOffectX = sValue.getIntValue();
				}
				else if (sItem == "offsety") {
					pcTEffectAge->_iOffectY = sValue.getIntValue();
				}
				else if (sItem == "alpha") {
					pcTEffectAge->_iAlpha = sValue.getIntValue();
				}
				else if (sItem == "rotation") {
					pcTEffectAge->_fRotation = (float)fn_atof(sValue.toUTF8().getAddress());
				}
				else if (sItem == "needtimer") {
					pcTEffectAge->_iNeedTimer = sValue.getIntValue();
				}
			}
		}
	}
}

void Control::triggerEffects(TEffectAge* pTEffectAge)
{
	TEffectAge* pcTEffect = pTEffectAge ? pTEffectAge : &_curEffects;

	if (getManager() != 0) {
		_anim.setParams(UIANIMTYPE_FLAT, 0, pcTEffect->_iNeedTimer, pcTEffect->_dFillingBK, pcTEffect->_dFillingBK, GetPos(), pcTEffect->_iOffectX, pcTEffect->_iOffectY, pcTEffect->_iZoom, pcTEffect->_iAlpha, pcTEffect->_fRotation);
		getManager()->addAnimationJob(&_anim);
	}
}

#endif // ZGUI_USE_ANIMATION

bool Control::IsVisible() const
{

    return m_bVisible && m_bInternVisible;
}

void Control::SetVisible(bool bVisible)
{
	if (m_bVisible == bVisible) {
		return;
	}

    bool v = IsVisible();
    m_bVisible = bVisible;
    if (m_bFocused) {
        m_bFocused = false;
    }
	if (!bVisible && _pManager && _pManager->GetFocus() == this) {
		_pManager->SetFocus(0);
	}
    if (IsVisible() != v) {
        NeedParentUpdate();
    }
}

void Control::SetInternVisible(bool bVisible)
{
    m_bInternVisible = bVisible;
	if (!bVisible && _pManager && _pManager->GetFocus() == this) {
		_pManager->SetFocus(0) ;
	}
}

bool Control::IsEnabled() const
{
    return m_bEnabled;
}

void Control::SetEnabled(bool bEnabled)
{
	if (m_bEnabled == bEnabled) {
		return;
	}

    m_bEnabled = bEnabled;
    Invalidate();
}

bool Control::IsMouseEnabled() const
{
    return m_bMouseEnabled;
}

void Control::SetMouseEnabled(bool bEnabled)
{
    m_bMouseEnabled = bEnabled;
}

bool Control::IsKeyboardEnabled() const
{
	return m_bKeyboardEnabled ;
}
void Control::SetKeyboardEnabled(bool bEnabled)
{
	m_bKeyboardEnabled = bEnabled ; 
}

bool Control::IsFocused() const
{
    return m_bFocused;
}

void Control::SetFocus()
{
    if( _pManager != NULL ) _pManager->SetFocus(this);
}

bool Control::IsFloat() const
{
    return _bFloat;
}

void Control::SetFloat(bool bFloat)
{
    if (_bFloat != bFloat) {
        _bFloat = bFloat;
        NeedParentUpdate();
    }
}

Control* Control::FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags)
{
    if ((uFlags & UIFIND_VISIBLE) != 0 && !IsVisible()) {
        return NULL;
    }
    if ((uFlags & UIFIND_ENABLED) != 0 && !IsEnabled()) {
        return NULL;
    }
    if ((uFlags & UIFIND_HITTEST) != 0 && (!m_bMouseEnabled || !fn_PtInRect(&_rcItem, * static_cast<LPPOINT>(pData)))) {
        return NULL;
    }

    return Proc(this, pData);
}

void Control::Invalidate()
{
    if (IsVisible()) {
        RECT invalidateRc = _rcItem;

        Control* pParent = this;
        RECT rcTemp;
        RECT rcParent;
        while (pParent = pParent->GetParent()) {
            rcTemp = invalidateRc;
            rcParent = pParent->GetPos();
            if (!fn_IntersectRect(&invalidateRc, &rcTemp, &rcParent)) {
                return;
            }
        }

        if (_pManager != NULL) {
            _pManager->Invalidate(invalidateRc);
        }
    }
}

bool Control::IsUpdateNeeded() const
{
    return m_bUpdateNeeded;
}

void Control::NeedUpdate()
{
    if (IsVisible()) {
        m_bUpdateNeeded = true;
        Invalidate();

        if (_pManager != NULL) {
            _pManager->NeedUpdate();
        }
    }
}

void Control::NeedParentUpdate()
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

uint32_t Control::GetAdjustColor(uint32_t dwColor)
{
    if( !m_bColorHSL ) return dwColor;
    short H, S, L;
    PaintManager::GetHSL(&H, &S, &L);
    return RenderEngine::AdjustColor(dwColor, H, S, L);
}

void Control::Init()
{
    DoInit();
    if( OnInit ) OnInit(this);
}

void Control::DoInit()
{

}

void Control::Event(TEventUI& event)
{
    if( OnEvent(&event) ) DoEvent(event);
}

void Control::DoEvent(TEventUI& event)
{
	if (event.Type == UIEVENT_SETCURSOR) {
        fn_SetCursor(fn_LoadCursorW(NULL, MAKEINTRESOURCE(IDC_ARROW)));
        return;
    }
	if (event.Type == UIEVENT_SETFOCUS) {
        m_bFocused = true;
        Invalidate();
        return;
    }
	if (event.Type == UIEVENT_KILLFOCUS) {
        m_bFocused = false;
        Invalidate();
        return;
    }
	if (event.Type == UIEVENT_TIMER) {
        _pManager->SendNotify(this, ZGUI_MSGTYPE_TIMER, event.wParam, event.lParam);
        return;
    }
	if (event.Type == UIEVENT_CONTEXTMENU) {
        if( IsContextMenuUsed() ) {
            _pManager->SendNotify(this, ZGUI_MSGTYPE_MENU, event.wParam, event.lParam);
            return;
        }
    }
    if (_pParent != NULL) {
        _pParent->DoEvent(event);
    }
}

void Control::SetVirtualWnd(const String& pstrValue)
{
    m_sVirtualWnd = pstrValue;
    _pManager->UsedVirtualWnd(true);
}

String Control::GetVirtualWnd() const
{
    String str;
    if (!m_sVirtualWnd.isEmpty()) {
        str = m_sVirtualWnd;
    }
    else {
        Control* pParent = GetParent();
        if (pParent != NULL) {
            str = pParent->GetVirtualWnd();
        }
        else {
            str = String::empty;
        }
    }
    return str;
}

void Control::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "pos") {
        RECT rcPos;
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
        RECT rcPadding;
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
	else if (pstrName == "disabledbordercolor") {
		SetDisabledBorderColor((uint32_t)pstrValue.getHexValue32());
	}
    else if (pstrName == "focusbordercolor") {
        SetFocusBorderColor((uint32_t)pstrValue.getHexValue32());
    }
    else if (pstrName == "colorhsl") {
        SetColorHSL(pstrValue == "true");
    }
    else if (pstrName == "bordersize") {
		RECT rcPadding;
		__stosb((uint8_t*)&rcPadding, 0, sizeof(rcPadding));

		if (pstrValue.indexOf(",") == -1) {
            SetBorderSize(pstrValue.getIntValue());
			SetBorderSize(rcPadding);
		}
        else {
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
        SIZE cxyRound;
        __stosb((uint8_t*)&cxyRound, 0, sizeof(cxyRound));
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
        setText(pstrValue.trim());
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
#ifdef ZGUI_USE_ANIMATION
	else if (pstrName == "adveffects") {
		setEffectsStyle(pstrValue, &_curEffects);
	}
#endif // ZGUI_USE_ANIMATION
}

Control* Control::applyAttributeList(const String& attrList)
{
    LPCTSTR pstrList = attrList.toWideCharPointer();
    String sItem;
    String sValue;

    while (*pstrList != L'\0') {
        sItem = String::empty;
        sValue = String::empty;
        while (*pstrList != L'\0' && *pstrList != L'=') {
            LPTSTR pstrTemp = fn_CharNextW(pstrList);
            while (pstrList < pstrTemp) {
                sItem += *pstrList++;
            }
        }
        zgui_assert(*pstrList == L'=');
        if (*pstrList++ != L'=') {
            return this;
        }

        zgui_assert( *pstrList == L'\"');
        if (*pstrList++ != L'\"') {
            return this;
        }

        while (*pstrList != L'\0' && *pstrList != L'\"') {
            LPTSTR pstrTemp = fn_CharNextW(pstrList);
            while (pstrList < pstrTemp) {
                sValue += *pstrList++;
            }
        }
        zgui_assert(*pstrList == L'\"');
        if (*pstrList++ != L'\"') {
            return this;
        }

        sItem = sItem.trim();
        sValue = sValue.trim();
        setAttribute(sItem, sValue);
        if (*pstrList++ != L' ') {
            return this;
        }
    }
    return this;
}

SIZE Control::EstimateSize(SIZE szAvailable)
{
    return _cxyFixed;
}

void Control::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if (!fn_IntersectRect(&_rcPaint, &rcPaint, &_rcItem)) {
        return;
    }

    if (m_cxyBorderRound.cx > 0 || m_cxyBorderRound.cy > 0) {
        RenderClip roundClip;
        RenderClip::GenerateRoundClip(hDC, _rcPaint,  _rcItem, m_cxyBorderRound.cx, m_cxyBorderRound.cy, roundClip);
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

void Control::PaintBkColor(HDC hDC)
{
    if (_dwBackColor != 0) {
        if (_dwBackColor2 != 0) {
            if (_dwBackColor3 != 0) {
                RECT rc = _rcItem; 

                if (_gradientVertical) {
                    rc.bottom = (rc.bottom + rc.top) / 2; 
                    RenderEngine::DrawGradient(hDC, rc, GetAdjustColor(_dwBackColor), GetAdjustColor(_dwBackColor2), _gradientVertical, _gradientSteps);
                    rc.top = rc.bottom;
                    rc.bottom = _rcItem.bottom;
                    RenderEngine::DrawGradient(hDC, rc, GetAdjustColor(_dwBackColor2), GetAdjustColor(_dwBackColor3), _gradientVertical, _gradientSteps);
                }
                else {
                    rc.right = (rc.right + rc.left) / 2; 
                    RenderEngine::DrawGradient(hDC, rc, GetAdjustColor(_dwBackColor), GetAdjustColor(_dwBackColor2), _gradientVertical, _gradientSteps);
                    rc.left = rc.right;
                    rc.right = _rcItem.right;
                    RenderEngine::DrawGradient(hDC, rc, GetAdjustColor(_dwBackColor2), GetAdjustColor(_dwBackColor3), _gradientVertical, _gradientSteps);
                }
            }
            else {
                RenderEngine::DrawGradient(hDC, _rcItem, GetAdjustColor(_dwBackColor), GetAdjustColor(_dwBackColor2), _gradientVertical, _gradientSteps);
            }
        }
        else if (_dwBackColor >= 0xFF000000) {
            RenderEngine::DrawColor(hDC, _rcPaint, GetAdjustColor(_dwBackColor));
        }
        else {
            RenderEngine::DrawColor(hDC, _rcItem, GetAdjustColor(_dwBackColor));
        }
    }
}

void Control::PaintBkImage(HDC hDC)
{
    if (!_bkImageName.isEmpty()) {
		if (!DrawImage(hDC, _bkImageName)) {
			_bkImageName = String::empty;
		}
    }
}

void Control::PaintStatusImage(HDC hDC)
{
    return;
}

void Control::PaintText(HDC hDC)
{
    return;
}

void Control::PaintBorder(HDC hDC)
{
	uint32_t iBorderColor;
	if (IsEnabled()) {
		iBorderColor = _dwBorderColor;
	}
	else {
		iBorderColor = _dwDisabledBorderColor;
	}
	if (iBorderColor != 0 || _dwFocusBorderColor != 0) {
        if (m_nBorderSize > 0 && ( m_cxyBorderRound.cx > 0 || m_cxyBorderRound.cy > 0 )) {
            if (IsFocused() && _dwFocusBorderColor != 0) {
				RenderEngine::DrawRoundRect(hDC, _rcItem, m_nBorderSize, m_cxyBorderRound.cx, m_cxyBorderRound.cy, GetAdjustColor(_dwFocusBorderColor));
            }
            else {
				RenderEngine::DrawRoundRect(hDC, _rcItem, m_nBorderSize, m_cxyBorderRound.cx, m_cxyBorderRound.cy, GetAdjustColor(iBorderColor));
            }
		}
		else {
            if (IsFocused() && _dwFocusBorderColor != 0 && m_nBorderSize > 0) {
				RenderEngine::DrawRect(hDC, _rcItem, m_nBorderSize, GetAdjustColor(_dwFocusBorderColor));
            }
            else if (m_rcBorderSize.left > 0 || m_rcBorderSize.top > 0 || m_rcBorderSize.right > 0 || m_rcBorderSize.bottom > 0) {
                RECT rcBorder;

                if (m_rcBorderSize.left > 0) {
                    rcBorder = _rcItem;
                    rcBorder.right = _rcItem.left;
					RenderEngine::DrawLine(hDC, rcBorder, m_rcBorderSize.left, GetAdjustColor(iBorderColor), m_nBorderStyle);
                }
                if (m_rcBorderSize.top > 0) {
                    rcBorder = _rcItem;
                    rcBorder.bottom	= _rcItem.top;
					RenderEngine::DrawLine(hDC, rcBorder, m_rcBorderSize.top, GetAdjustColor(iBorderColor), m_nBorderStyle);
                }
                if (m_rcBorderSize.right > 0) {
                    rcBorder = _rcItem;
					rcBorder.left = --rcBorder.right;
					RenderEngine::DrawLine(hDC, rcBorder, m_rcBorderSize.right, GetAdjustColor(iBorderColor), m_nBorderStyle);
                }
                if (m_rcBorderSize.bottom > 0) {
                    rcBorder = _rcItem;
					rcBorder.top = --rcBorder.bottom;
					RenderEngine::DrawLine(hDC, rcBorder, m_rcBorderSize.bottom, GetAdjustColor(iBorderColor), m_nBorderStyle);
                }
            }
            else if (m_nBorderSize > 0) {
				RenderEngine::DrawRect(hDC, _rcItem, m_nBorderSize, GetAdjustColor(iBorderColor));
            }
		}
	}
}

void Control::DoPostPaint(HDC hDC, const RECT& rcPaint)
{
    return;
}

int Control::GetLeftBorderSize() const
{
	return m_rcBorderSize.left;
}

void Control::SetLeftBorderSize( int nSize )
{
	m_rcBorderSize.left = nSize;
	Invalidate();
}

int Control::GetTopBorderSize() const
{
	return m_rcBorderSize.top;
}

void Control::SetTopBorderSize( int nSize )
{
	m_rcBorderSize.top = nSize;
	Invalidate();
}

int Control::GetRightBorderSize() const
{
	return m_rcBorderSize.right;
}

void Control::SetRightBorderSize( int nSize )
{
	m_rcBorderSize.right = nSize;
	Invalidate();
}

int Control::GetBottomBorderSize() const
{
	return m_rcBorderSize.bottom;
}

void Control::SetBottomBorderSize( int nSize )
{
	m_rcBorderSize.bottom = nSize;
	Invalidate();
}

int Control::GetBorderStyle() const
{
	return m_nBorderStyle;
}

void Control::SetBorderStyle( int nStyle )
{
	m_nBorderStyle = nStyle;
	Invalidate();
}

} // namespace zgui
