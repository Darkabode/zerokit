#include "zgui.h"

namespace zgui {

DelegateBase::DelegateBase(void* pObject, void* pFn) 
{
    m_pObject = pObject;
    m_pFn = pFn; 
}

DelegateBase::DelegateBase(const DelegateBase& rhs) 
{
    m_pObject = rhs.m_pObject;
    m_pFn = rhs.m_pFn; 
}

DelegateBase::~DelegateBase()
{

}

bool DelegateBase::Equals(const DelegateBase& rhs) const 
{
    return m_pObject == rhs.m_pObject && m_pFn == rhs.m_pFn; 
}

bool DelegateBase::operator() (void* param) 
{
    return Invoke(param); 
}

void* DelegateBase::GetFn() 
{
    return m_pFn; 
}

void* DelegateBase::GetObject() 
{
    return m_pObject; 
}

EventSource::~EventSource()
{
    for (int i = 0; i < _aDelegates.size(); ++i) {
        DelegateBase* pObject = static_cast<DelegateBase*>(_aDelegates[i]);
		if (pObject) {
			delete pObject;
		}
    }
}

EventSource::operator bool()
{
    return _aDelegates.size() > 0;
}

void EventSource::operator+= (const DelegateBase& d)
{ 
    for (int i = 0; i < _aDelegates.size(); ++i) {
        DelegateBase* pObject = static_cast<DelegateBase*>(_aDelegates[i]);
		if (pObject && pObject->Equals(d)) {
			return;
		}
    }

    _aDelegates.add(d.Copy());
}

void EventSource::operator+= (FnType pFn)
{ 
    (*this) += MakeDelegate(pFn);
}

void EventSource::operator-= (const DelegateBase& d) 
{
    for (int i = 0; i < _aDelegates.size(); ++i) {
        DelegateBase* pObject = static_cast<DelegateBase*>(_aDelegates[i]);
        if (pObject && pObject->Equals(d)) {
            delete pObject;
            _aDelegates.remove(i);
            return;
        }
    }
}
void EventSource::operator-= (FnType pFn)
{ 
    (*this) -= MakeDelegate(pFn);
}

bool EventSource::operator() (void* param) 
{
    for (int i = 0; i < _aDelegates.size(); ++i) {
        DelegateBase* pObject = static_cast<DelegateBase*>(_aDelegates[i]);
		if (pObject && !(*pObject)(param)) {
			return false;
		}
    }
    return true;
}

} // namespace zgui
