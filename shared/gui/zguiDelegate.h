#ifndef __ZGUI_DELEGATE_H_
#define __ZGUI_DELEGATE_H_

namespace zgui {

class DelegateBase	 
{
public:
    DelegateBase(void* pObject, void* pFn);
    DelegateBase(const DelegateBase& rhs);
    virtual ~DelegateBase();
    bool Equals(const DelegateBase& rhs) const;
    bool operator() (void* param);
    virtual DelegateBase* Copy() const = 0; // add const for gcc

protected:
    void* GetFn();
    void* GetObject();
    virtual bool Invoke(void* param) = 0;

private:
    void* m_pObject;
    void* m_pFn;
};

class DelegateStatic: public DelegateBase
{
    typedef bool (*Fn)(void*);
public:
    DelegateStatic(Fn pFn) : DelegateBase(NULL, pFn) { } 
    DelegateStatic(const DelegateStatic& rhs) : DelegateBase(rhs) { } 
    virtual DelegateBase* Copy() const { return new DelegateStatic(*this); }

protected:
    virtual bool Invoke(void* param)
    {
        Fn pFn = (Fn)GetFn();
        return (*pFn)(param); 
    }
};

template <class O, class T>
class Delegate : public DelegateBase
{
    typedef bool (T::* Fn)(void*);
public:
    Delegate(O* pObj, Fn pFn) : DelegateBase(pObj, &pFn), m_pFn(pFn) { }
    Delegate(const Delegate& rhs) : DelegateBase(rhs) { m_pFn = rhs.m_pFn; } 
    virtual DelegateBase* Copy() const { return new Delegate(*this); }

protected:
    virtual bool Invoke(void* param)
    {
        O* pObject = (O*) GetObject();
        return (pObject->*m_pFn)(param); 
    }  

private:
    Fn m_pFn;
};

template <class O, class T>
Delegate<O, T> MakeDelegate(O* pObject, bool (T::* pFn)(void*))
{
    return Delegate<O, T>(pObject, pFn);
}

inline DelegateStatic MakeDelegate(bool (*pFn)(void*))
{
    return DelegateStatic(pFn); 
}

class EventSource
{
    typedef bool (*FnType)(void*);
public:
    ~EventSource();
    operator bool();
    void operator+=(const DelegateBase& d);
    void operator+=(FnType pFn);
    void operator-=(const DelegateBase& d);
    void operator-=(FnType pFn);
    bool operator()(void* param);

protected:
	Array<void*> _aDelegates;
};

} // namespace zgui

#endif // __ZGUI_DELEGATE_H_