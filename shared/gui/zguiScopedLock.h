#ifndef __ZGUI_SCOPEDLOCK_H_
#define __ZGUI_SCOPEDLOCK_H_


namespace zgui
{
/**
    Automatically locks and unlocks a mutex object.

    Use one of these as a local variable to provide RAII-based locking of a mutex.

    The templated class could be a CriticalSection, SpinLock, or anything else that
    provides enter() and exit() methods.

    e.g. @code
    CriticalSection myCriticalSection;

    for (;;)
    {
        const GenericScopedLock<CriticalSection> myScopedLock (myCriticalSection);
        // myCriticalSection is now locked

        ...do some stuff...

        // myCriticalSection gets unlocked here.
    }
    @endcode

    @see GenericScopedUnlock, CriticalSection, SpinLock, ScopedLock, ScopedUnlock
*/
template <class LockType>
class GenericScopedLock
{
public:
    /** Creates a GenericScopedLock.

        As soon as it is created, this will acquire the lock, and when the GenericScopedLock
        object is deleted, the lock will be released.

        Make sure this object is created and deleted by the same thread,
        otherwise there are no guarantees what will happen! Best just to use it
        as a local stack object, rather than creating one with the new() operator.
    */
    inline explicit GenericScopedLock(LockType& lock) throw() :
    _lock(lock)
    {
        lock.enter();
    }

    /** Destructor.
        The lock will be released when the destructor is called.
        Make sure this object is created and deleted by the same thread, otherwise there are
        no guarantees what will happen!
    */
    inline ~GenericScopedLock() throw()
    {
        _lock.exit();
    }

private:
    LockType& _lock;

    ZGUI_DECLARE_NON_COPYABLE(GenericScopedLock);
};


/**
    Automatically unlocks and re-locks a mutex object.

    This is the reverse of a GenericScopedLock object - instead of locking the mutex
    for the lifetime of this object, it unlocks it.

    Make sure you don't try to unlock mutexes that aren't actually locked!

    e.g. @code

    CriticalSection myCriticalSection;

    for (;;)
    {
        const GenericScopedLock<CriticalSection> myScopedLock (myCriticalSection);
        // myCriticalSection is now locked

        ... do some stuff with it locked ..

        while (xyz)
        {
            ... do some stuff with it locked ..

            const GenericScopedUnlock<CriticalSection> unlocker (myCriticalSection);

            // myCriticalSection is now unlocked for the remainder of this block,
            // and re-locked at the end.

            ...do some stuff with it unlocked ...
        }

        // myCriticalSection gets unlocked here.
    }
    @endcode

    @see GenericScopedLock, CriticalSection, ScopedLock, ScopedUnlock
*/
template <class LockType>
class GenericScopedUnlock
{
public:
    /** Creates a GenericScopedUnlock.

        As soon as it is created, this will unlock the CriticalSection, and
        when the ScopedLock object is deleted, the CriticalSection will
        be re-locked.

        Make sure this object is created and deleted by the same thread,
        otherwise there are no guarantees what will happen! Best just to use it
        as a local stack object, rather than creating one with the new() operator.
    */
    inline explicit GenericScopedUnlock(LockType& lock) throw() :
    _lock(lock)
    {
        lock.exit();
    }

    /** Destructor.

        The CriticalSection will be unlocked when the destructor is called.

        Make sure this object is created and deleted by the same thread,
        otherwise there are no guarantees what will happen!
    */
    inline ~GenericScopedUnlock() throw()
    {
        _lock.enter();
    }


private:
    LockType& _lock;

    ZGUI_DECLARE_NON_COPYABLE(GenericScopedUnlock);
};


/**
    Automatically locks and unlocks a mutex object.

    Use one of these as a local variable to provide RAII-based locking of a mutex.

    The templated class could be a CriticalSection, SpinLock, or anything else that
    provides enter() and exit() methods.

    e.g. @code

    CriticalSection myCriticalSection;

    for (;;)
    {
        const GenericScopedTryLock<CriticalSection> myScopedTryLock (myCriticalSection);

        // Unlike using a ScopedLock, this may fail to actually get the lock, so you
        // should test this with the isLocked() method before doing your thread-unsafe
        // action..
        if (myScopedTryLock.isLocked())
        {
           ...do some stuff...
        }
        else
        {
            ..our attempt at locking failed because another thread had already locked it..
        }

        // myCriticalSection gets unlocked here (if it was locked)
    }
    @endcode

    @see CriticalSection::tryEnter, GenericScopedLock, GenericScopedUnlock
*/
template <class LockType>
class GenericScopedTryLock
{
public:
    /** Creates a GenericScopedTryLock.

        As soon as it is created, this will attempt to acquire the lock, and when the
        GenericScopedTryLock is deleted, the lock will be released (if the lock was
        successfully acquired).

        Make sure this object is created and deleted by the same thread,
        otherwise there are no guarantees what will happen! Best just to use it
        as a local stack object, rather than creating one with the new() operator.
    */
    inline explicit GenericScopedTryLock(LockType& lock) throw() :
    _lock(lock),
    lockWasSuccessful(lock.tryEnter())
    {
    }

    /** Destructor.

        The mutex will be unlocked (if it had been successfully locked) when the
        destructor is called.

        Make sure this object is created and deleted by the same thread,
        otherwise there are no guarantees what will happen!
    */
    inline ~GenericScopedTryLock() throw()
    {
        if (lockWasSuccessful) {
            _lock.exit();
        }
    }

    /** Returns true if the mutex was successfully locked. */
    bool isLocked() const throw()
    {
        return lockWasSuccessful;
    }

private:
    LockType& _lock;
    bool lockWasSuccessful;

    ZGUI_DECLARE_NON_COPYABLE(GenericScopedTryLock);
};

}

#endif   // __JUCE_SCOPEDLOCK_JUCEHEADER__
