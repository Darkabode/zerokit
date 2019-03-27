#ifndef __JUCE_WAITABLEEVENT_JUCEHEADER__
#define __JUCE_WAITABLEEVENT_JUCEHEADER__

namespace zgui
{

/**
    Allows threads to wait for events triggered by other threads.

    A thread can call wait() on a WaitableObject, and this will suspend the
    calling thread until another thread wakes it up by calling the signal()
    method.
*/
class WaitableEvent
{
public:
    /** Creates a WaitableEvent object.

        @param manualReset  If this is false, the event will be reset automatically when the wait()
                            method is called. If manualReset is true, then once the event is signalled,
                            the only way to reset it will be by calling the reset() method.
    */
    WaitableEvent (bool manualReset = false) throw();

    /** Destructor.

        If other threads are waiting on this object when it gets deleted, this
        can cause nasty errors, so be careful!
    */
    ~WaitableEvent() throw();

    /** Suspends the calling thread until the event has been signalled.

        This will wait until the object's signal() method is called by another thread,
        or until the timeout expires.

        After the event has been signalled, this method will return true and if manualReset
        was set to false in the WaitableEvent's constructor, then the event will be reset.

        @param timeOutMilliseconds  the maximum time to wait, in milliseconds. A negative
                                    value will cause it to wait forever.

        @returns    true if the object has been signalled, false if the timeout expires first.
        @see signal, reset
    */
    bool wait (int timeOutMilliseconds = -1) const throw();

    /** Wakes up any threads that are currently waiting on this object.

        If signal() is called when nothing is waiting, the next thread to call wait()
        will return immediately and reset the signal.

        If the WaitableEvent is manual reset, all current and future threads that wait upon this
        object will be woken, until reset() is explicitly called.

        If the WaitableEvent is automatic reset, and one or more threads is waiting upon the object,
        then one of them will be woken up. If no threads are currently waiting, then the next thread
        to call wait() will be woken up. As soon as a thread is woken, the signal is automatically
        reset.

        @see wait, reset
    */
    void signal() const throw();

    /** Resets the event to an unsignalled state.

        If it's not already signalled, this does nothing.
    */
    void reset() const throw();


private:
    void* _internal;

    ZGUI_DECLARE_NON_COPYABLE(WaitableEvent);
};

}

#endif   // __JUCE_WAITABLEEVENT_JUCEHEADER__
