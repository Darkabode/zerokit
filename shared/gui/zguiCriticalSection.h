#ifndef __ZGUI_CRITICALSECTION_H_
#define __ZGUI_CRITICALSECTION_H_

namespace zgui {

/**
    A mutex class.

    A CriticalSection acts as a re-entrant mutex lock. The best way to lock and unlock
    one of these is by using RAII in the form of a local ScopedLock object - have a look
    through the codebase for many examples of how to do this.

    @see ScopedLock, ScopedTryLock, ScopedUnlock, SpinLock, ReadWriteLock, Thread, InterProcessLock
*/
class CriticalSection
{
public:
    /** Creates a CriticalSection object. */
    CriticalSection() throw();

    /** Destructor.
        If the critical section is deleted whilst locked, any subsequent behaviour
        is unpredictable.
    */
    ~CriticalSection() throw();

    /** Acquires the lock.

        If the lock is already held by the caller thread, the method returns immediately.
        If the lock is currently held by another thread, this will wait until it becomes free.

        It's strongly recommended that you never call this method directly - instead use the
        ScopedLock class to manage the locking using an RAII pattern instead.

        @see exit, tryEnter, ScopedLock
    */
    void enter() throw();

    /** Attempts to lock this critical section without blocking.

        This method behaves identically to CriticalSection::enter, except that the caller thread
        does not wait if the lock is currently held by another thread but returns false immediately.

        @returns false if the lock is currently held by another thread, true otherwise.
        @see enter
    */
    bool tryEnter() throw();

    /** Releases the lock.

        If the caller thread hasn't got the lock, this can have unpredictable results.

        If the enter() method has been called multiple times by the thread, each
        call must be matched by a call to exit() before other threads will be allowed
        to take over the lock.

        @see enter, ScopedLock
    */
    void exit() throw();

    /** Provides the type of scoped lock to use with a CriticalSection. */
    typedef GenericScopedLock<CriticalSection> ScopedLockType;

    /** Provides the type of scoped unlocker to use with a CriticalSection. */
    typedef GenericScopedUnlock<CriticalSection> ScopedUnlockType;

    /** Provides the type of scoped try-locker to use with a CriticalSection. */
    typedef GenericScopedTryLock<CriticalSection> ScopedTryLockType;


private:
    CRITICAL_SECTION _critSect;

    ZGUI_DECLARE_NON_COPYABLE(CriticalSection);
};


/**
    A class that can be used in place of a real CriticalSection object, but which
    doesn't perform any locking.

    This is currently used by some templated classes, and most compilers should
    manage to optimise it out of existence.

    @see CriticalSection, Array, OwnedArray, ReferenceCountedArray
*/
class DummyCriticalSection
{
public:
    inline DummyCriticalSection() throw()
    {
    }

    inline ~DummyCriticalSection() throw()
    {
    }

    inline void enter() const throw()
    {
    }

    inline bool tryEnter() const throw()
    {
        return true;
    }

    inline void exit() const throw()
    {
    }

    /** A dummy scoped-lock type to use with a dummy critical section. */
    struct ScopedLockType
    {
        ScopedLockType(const DummyCriticalSection&) throw()
        {
        }
    };

    /** A dummy scoped-unlocker type to use with a dummy critical section. */
    typedef ScopedLockType ScopedUnlockType;

private:
    ZGUI_DECLARE_NON_COPYABLE(DummyCriticalSection);
};

/**
    Automatically locks and unlocks a CriticalSection object.

    Use one of these as a local variable to provide RAII-based locking of a CriticalSection.

    e.g. @code

    CriticalSection myCriticalSection;

    for (;;)
    {
        const ScopedLock myScopedLock (myCriticalSection);
        // myCriticalSection is now locked

        ...do some stuff...

        // myCriticalSection gets unlocked here.
    }
    @endcode

    @see CriticalSection, ScopedUnlock
*/
typedef CriticalSection::ScopedLockType ScopedLock;

/**
    Automatically unlocks and re-locks a CriticalSection object.

    This is the reverse of a ScopedLock object - instead of locking the critical
    section for the lifetime of this object, it unlocks it.

    Make sure you don't try to unlock critical sections that aren't actually locked!

    e.g. @code

    CriticalSection myCriticalSection;

    for (;;)
    {
        const ScopedLock myScopedLock (myCriticalSection);
        // myCriticalSection is now locked

        ... do some stuff with it locked ..

        while (xyz)
        {
            ... do some stuff with it locked ..

            const ScopedUnlock unlocker (myCriticalSection);

            // myCriticalSection is now unlocked for the remainder of this block,
            // and re-locked at the end.

            ...do some stuff with it unlocked ...
        }

        // myCriticalSection gets unlocked here.
    }
    @endcode

    @see CriticalSection, ScopedLock
*/
typedef CriticalSection::ScopedUnlockType ScopedUnlock;

/**
    Automatically tries to lock and unlock a CriticalSection object.

    Use one of these as a local variable to control access to a CriticalSection.

    e.g. @code
    CriticalSection myCriticalSection;

    for (;;)
    {
        const ScopedTryLock myScopedTryLock (myCriticalSection);

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

    @see CriticalSection::tryEnter, ScopedLock, ScopedUnlock, ScopedReadLock
*/
typedef CriticalSection::ScopedTryLockType ScopedTryLock;

}
#endif // __ZGUI_CRITICALSECTION_H_
