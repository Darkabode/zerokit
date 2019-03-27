#include "zgui.h"

#ifdef ZGUI_USE_TABLAYOUT

namespace zgui {

const String TabLayout::CLASS_NAME = ZGUI_TABLAYOUT;

TabLayout::TabLayout() :
_iCurSel(-1),
_animType(ANIM_NONE)
{
}

const String& TabLayout::getClass() const
{
	return CLASS_NAME;
}

LPVOID TabLayout::getInterface(const String& name)
{
	if (name == ZGUI_TABLAYOUT) {
        return static_cast<TabLayout*>(this);
    }
	return Container::getInterface(name);
}

bool TabLayout::add(Control* pControl)
{
	bool ret = Container::add(pControl);
	if (!ret) {
		return ret;
	}

	if (_iCurSel == -1 && pControl->IsVisible()) {
		_iCurSel = indexOf(pControl);
	}
	else {
		pControl->SetVisible(false);
	}

	return ret;
}

bool TabLayout::insert(int iIndex, Control* pControl)
{
	bool ret = Container::insert(iIndex, pControl);
	if (!ret) {
		return ret;
	}

	if (_iCurSel == -1 && pControl->IsVisible()) {
		_iCurSel = indexOf(pControl);
	}
	else if( _iCurSel != -1 && iIndex <= _iCurSel) {
		_iCurSel += 1;
	}
	else {
		pControl->SetVisible(false);
	}

	return ret;
}

bool TabLayout::remove(Control* pControl)
{
	if( pControl == NULL) return false;

	int index = indexOf(pControl);
	bool ret = Container::remove(pControl);
	if( !ret ) return false;

	if( _iCurSel == index)
	{
		if (getCount() > 0) 
            getItem(0)->SetVisible(true);
        else
            _iCurSel=-1;
		NeedParentUpdate();
	}
	else if( _iCurSel > index )
	{
		_iCurSel -= 1;
	}

	return ret;
}

void TabLayout::removeAll()
{
	_iCurSel = -1;
	Container::removeAll();
	NeedParentUpdate();
}

int TabLayout::GetCurSel() const
{
	return _iCurSel;
}

bool TabLayout::selectItem(int iIndex)
{
	String anim;
	if (iIndex < 0 || iIndex >= _items.size()) {
		return false;
	}
	if (iIndex == _iCurSel) {
		return true;
	}

	_iOldSel = _iCurSel;
	_iCurSel = iIndex;
#ifdef ZGUI_USE_ANIMATION
	if (_animType != ANIM_NONE) {
		Control* pItem = getItem(iIndex);
		pItem->getAnimation().onAnimationFinished += zgui::MakeDelegate(this, &TabLayout::onAnimationFinished);
		switch (_animType) {
			case ANIM_HORIZONTAL:
				if (iIndex > _iOldSel) {
					anim = "anim='left2right' offset='180'";
				}
				else {
					anim = "anim='right2left' offset='180'";
				}
				break;
			case ANIM_VERTICAL:
				if (iIndex > _iOldSel) {
					anim = "anim='bottom2top' offset='180'";
				}
				else {
					anim = "anim='top2bottom' offset='180'";
				}
				break;
		}

		pItem->setAttribute("adveffects", anim);
		pItem->triggerEffects();
	}
	else {
		onAnimationFinished(0);
	}
#else
	for (int it = 0; it < _items.size(); ++it) {
		if (it == _iCurSel) {
			getItem(it)->SetVisible(true);
			getItem(it)->SetFocus();
			SetPos(_rcItem);
		}
		else {
			getItem(it)->SetVisible(false);
		}
	}
	NeedParentUpdate();

	if (_pManager != 0) {
		_pManager->SetNextTabControl();
		_pManager->SendNotify(this, ZGUI_MSGTYPE_TABSELECT, _iCurSel, _iOldSel);
	}
#endif // ZGUI_USE_ANIMATION
	return true;
}
#ifdef ZGUI_USE_ANIMATION
bool TabLayout::onAnimationFinished(void* pParam)
{
	for (int it = 0; it < _items.size(); ++it) {
		if (it == _iCurSel) {
			getItem(it)->SetVisible(true);
			getItem(it)->SetFocus();
			SetPos(_rcItem);
		}
		else {
			getItem(it)->SetVisible(false);
		}
	}
	NeedParentUpdate();

	if (_pManager != 0) {
		_pManager->SetNextTabControl();
		_pManager->SendNotify(this, ZGUI_MSGTYPE_TABSELECT, _iCurSel, _iOldSel);
	}

	return true;
}
#endif // ZGUI_USE_ANIMATION
void TabLayout::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "selectedid") {
        selectItem(pstrValue.getIntValue());
    }
	else if (pstrName == "animation") {
		if (pstrValue == "horizontal") {
			_animType = ANIM_HORIZONTAL;
		}
		else if (pstrValue == "vertical") {
			_animType = ANIM_VERTICAL;
		}
	}
	else {
		Container::setAttribute(pstrName, pstrValue);
	}
}

void TabLayout::SetPos(RECT rc)
{
	Control::SetPos(rc);
	rc = _rcItem;

	// Adjust for inset
	rc.left += _rcInset.left;
	rc.top += _rcInset.top;
	rc.right -= _rcInset.right;
	rc.bottom -= _rcInset.bottom;

	for (int it = 0; it < _items.size(); ++it) {
		Control* pControl = _items.getUnchecked(it);
		// Не нужно для анимации!!!
		//if (!pControl->IsVisible()) {
		//	continue;
		//}
		if (pControl->IsFloat()) {
			SetFloatPos(it);
			continue;
		}

		// Не нужно для анимации!!!
		//if (it != _iCurSel) {
		//	continue;
		//}

		RECT rcPadding = pControl->GetPadding();
		rc.left += rcPadding.left;
		rc.top += rcPadding.top;
		rc.right -= rcPadding.right;
		rc.bottom -= rcPadding.bottom;

		SIZE szAvailable = {rc.right - rc.left, rc.bottom - rc.top};

		SIZE sz = pControl->EstimateSize(szAvailable);
		if (sz.cx == 0) {
			sz.cx = MAX(0, szAvailable.cx);
		}
		if (sz.cx < pControl->GetMinWidth()) {
			sz.cx = pControl->GetMinWidth();
		}
		if (sz.cx > pControl->GetMaxWidth()) {
			sz.cx = pControl->GetMaxWidth();
		}

		if (sz.cy == 0) {
			sz.cy = MAX(0, szAvailable.cy);
		}
		if (sz.cy < pControl->GetMinHeight()) {
			sz.cy = pControl->GetMinHeight();
		}
		if (sz.cy > pControl->GetMaxHeight()) {
			sz.cy = pControl->GetMaxHeight();
		}

		RECT rcCtrl = { rc.left, rc.top, rc.left + sz.cx, rc.top + sz.cy};
		pControl->SetPos(rcCtrl);
	}
}

}

#endif // ZGUI_USE_TABLAYOUT