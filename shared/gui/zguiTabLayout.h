#ifndef __ZGUI_TABLAYOUT_H_
#define __ZGUI_TABLAYOUT_H_

#ifdef ZGUI_USE_TABLAYOUT

namespace zgui
{

class TabLayout : public Container
{
public:
	TabLayout();

	const String& getClass() const;
	LPVOID getInterface(const String& name);

	bool add(Control* pControl);
	bool insert(int iIndex, Control* pControl);
	bool remove(Control* pControl);
	void removeAll();
	int GetCurSel() const;
	bool selectItem(int iIndex);

	void SetPos(RECT rc);

	void setAttribute(const String& pstrName, const String& pstrValue);

protected:
	int _iOldSel;
	int _iCurSel;
private:
#ifdef ZGUI_USE_ANIMATION
	bool onAnimationFinished(void* pParam);
#endif
	enum {
		ANIM_NONE = -1,
		ANIM_HORIZONTAL,
		ANIM_VERTICAL,
	};
	int _animType;
	static const String CLASS_NAME;
};

}

#endif // ZGUI_USE_TABLAYOUT

#endif // __ZGUI_TABLAYOUT_H_