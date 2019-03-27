#ifndef __ZGUI_SINGLETON_H_
#define __ZGUI_SINGLETON_H_

/**
    Macro to declare member variables and methods for a singleton class.

    To use this, add the line juce_DeclareSingleton (MyClass, doNotRecreateAfterDeletion)
    to the class's definition.

    Then put a macro juce_ImplementSingleton (MyClass) along with the class's
    implementation code.

    It's also a very good idea to also add the call clearSingletonInstance() in your class's
    destructor, in case it is deleted by other means than deleteInstance()

    Clients can then call the static method MyClass::getInstance() to get a pointer
    to the singleton, or MyClass::getInstanceWithoutCreating() which will return 0 if
    no instance currently exists.

    e.g. @code

        class MySingleton
        {
        public:
            MySingleton()
            {
            }

            ~MySingleton()
            {
                // this ensures that no dangling pointers are left when the
                // singleton is deleted.
                clearSingletonInstance();
            }

            juce_DeclareSingleton (MySingleton, false)
        };

        juce_ImplementSingleton (MySingleton)


        // example of usage:
        MySingleton* m = MySingleton::getInstance(); // creates the singleton if there isn't already one.

        ...

        MySingleton::deleteInstance(); // safely deletes the singleton (if it's been created).

    @endcode

    If doNotRecreateAfterDeletion = true, it won't allow the object to be created more
    than once during the process's lifetime - i.e. after you've created and deleted the
    object, getInstance() will refuse to create another one. This can be useful to stop
    objects being accidentally re-created during your app's shutdown code.

    If you know that your object will only be created and deleted by a single thread, you
    can use the slightly more efficient juce_DeclareSingleton_SingleThreaded() macro instead
    of this one.

    @see juce_ImplementSingleton, juce_DeclareSingleton_SingleThreaded
*/
#define zgui_DeclareSingleton(classname) \
\
    static classname* _singletonInstance;  \
    static zgui::CriticalSection _singletonLock; \
\
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
    }

/** This is a counterpart to the juce_DeclareSingleton macro.

    After adding the juce_DeclareSingleton to the class definition, this macro has
    to be used in the cpp file.
*/
#define zgui_ImplementSingleton(classname) \
\
    classname* classname::_singletonInstance = 0; \
    zgui::CriticalSection classname::_singletonLock;

#endif // __ZGUI_SINGLETON_H_
