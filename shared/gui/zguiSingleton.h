#ifndef __ZGUI_SINGLETON_H_
#define __ZGUI_SINGLETON_H_

#define ZGUI_DECLARE_SINGLETON(classname) \
    static classname* __stdcall getInstance() \
    { \
        const zgui::ScopedLock sl(_singletonLock); \
        if (_singletonInstance == 0) { \
            _singletonInstance = new classname(); \
        } \
        return _singletonInstance; \
    } \
\
    static inline classname* __stdcall getInstanceWithoutCreating() throw() \
    { \
        return _singletonInstance; \
    } \
\
    static void __stdcall deleteInstance() \
    { \
        const zgui::ScopedLock sl(_singletonLock); \
        if (_singletonInstance != 0) { \
            classname* const old = _singletonInstance; \
            _singletonInstance = 0; \
            delete old; \
        } \
    } \
\
    void clearSingletonInstance() throw()\
    { \
        if (_singletonInstance == this) \
            _singletonInstance = 0; \
    } \
	\
	static classname* _singletonInstance;  \
	static zgui::CriticalSection _singletonLock


#define ZGUI_IMPLEMENT_SINGLETON(classname) \
\
    classname* classname::_singletonInstance = 0; \
    zgui::CriticalSection classname::_singletonLock

#endif // __ZGUI_SINGLETON_H_
