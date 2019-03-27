#include "zgui.h"

namespace zgui
{

Thread::Thread() :
_threadHandle(0),
_threadId(0),
threadPriority(5),
affinityMask(0),
shouldExit(false)
{
}

Thread::~Thread()
{
    /* If your thread class's destructor has been called without first stopping the thread, that
       means that this partially destructed object is still performing some work - and that's
       probably a Bad Thing!

       To avoid this type of nastiness, always make sure you call stopThread() before or during
       your subclass's destructor.
    */
    stopThread(100);
}

// Use a ref-counted object to hold this shared data, so that it can outlive its static
// shared pointer when threads are still running during static shutdown.
class CurrentThreadHolder
{
public:
    CurrentThreadHolder() throw() :
    _slot(fn_TlsAlloc())
    {
    }

    ~CurrentThreadHolder()
    {
        fn_TlsFree(_slot);
    }

    Thread* get() const
    {
        return reinterpret_cast<Thread*>(fn_TlsGetValue(_slot));
    }
    void set(Thread* pThread)
    {
        fn_TlsSetValue(_slot, pThread);
    }

private:
    DWORD _slot;
};

static CurrentThreadHolder _currentThreadHolder;


void Thread::threadEntryPoint()
{
    _currentThreadHolder.set(this);

    if (startSuspensionEvent.wait (10000)) {
        if (affinityMask != 0) {
            setCurrentThreadAffinityMask(affinityMask);
        }

        run();
    }

    closeThreadHandle();
}

// used to wrap the incoming call from the platform-specific code
void zgui_threadEntryPoint (void* userData)
{
    static_cast<Thread*>(userData)->threadEntryPoint();
}

void Thread::startThread()
{
    const ScopedLock sl (startStopLock);

    shouldExit = false;

    if (_threadHandle == 0) {
        launchThread();
        setThreadPriority (_threadHandle, threadPriority);
        startSuspensionEvent.signal();
    }
}

void Thread::startThread (const int priority)
{
    const ScopedLock sl (startStopLock);

    if (_threadHandle == 0)
    {
        threadPriority = priority;
        startThread();
    }
    else
    {
        setPriority (priority);
    }
}

bool Thread::isThreadRunning() const
{
    return _threadHandle != 0;
}

Thread* Thread::getCurrentThread()
{
    return _currentThreadHolder.get();
}

void Thread::signalThreadShouldExit()
{
    shouldExit = true;
}

static uint32_t lastMSCounterValue = 0;

uint32_t getMillisecondCounter()
{
    const uint32_t now = (uint32_t)fn_timeGetTime();

    if (now < lastMSCounterValue) {
        // in multi-threaded apps this might be called concurrently, so
        // make sure that our last counter value only increases and doesn't
        // go backwards..
        if (now < lastMSCounterValue - 1000) {
            lastMSCounterValue = now;
        }
    }
    else {
        lastMSCounterValue = now;
    }

    return now;
}


bool Thread::waitForThreadToExit (const int timeOutMilliseconds) const
{
    const uint32_t timeoutEnd = getMillisecondCounter() + timeOutMilliseconds;

    while (isThreadRunning()) {
        if (timeOutMilliseconds >= 0 && getMillisecondCounter() > timeoutEnd) {
            return false;
        }
        sleep(2);
    }

    return true;
}

void Thread::stopThread (const int timeOutMilliseconds)
{
    const ScopedLock sl (startStopLock);

    if (isThreadRunning()) {
        signalThreadShouldExit();
		notifyThreads();

        if (timeOutMilliseconds != 0) {
            waitForThreadToExit (timeOutMilliseconds);
        }

        if (isThreadRunning()) {
            killThread();

            _threadHandle = 0;
            _threadId = 0;
        }
    }
}

bool Thread::setPriority(const int newPriority)
{
    // NB: deadlock possible if you try to set the thread prio from the thread itself,
    // so using setCurrentThreadPriority instead in that case.
    if (getCurrentThreadId() == getThreadId()) {
        return setCurrentThreadPriority (newPriority);
    }

    const ScopedLock sl (startStopLock);

    if (setThreadPriority (_threadHandle, newPriority)) {
        threadPriority = newPriority;
        return true;
    }

    return false;
}

bool Thread::setCurrentThreadPriority (const int newPriority)
{
    return setThreadPriority (0, newPriority);
}

void Thread::setAffinityMask (const uint32_t newAffinityMask)
{
    affinityMask = newAffinityMask;
}

bool Thread::wait (const int timeOutMilliseconds) const
{
    return _defaultEvent.wait(timeOutMilliseconds);
}

void Thread::notifyThreads() const
{
    _defaultEvent.signal();
}


void zgui_threadEntryPoint(void*);

static uint32_t __stdcall threadEntryProc(void* userData)
{
//     if (juce_messageWindowHandle != 0)
//         AttachThreadInput (GetWindowThreadProcessId(juce_messageWindowHandle, 0), GetCurrentThreadId(), TRUE);

    zgui_threadEntryPoint(userData);

    //_endthreadex (0);
    return 0;
}

void Thread::launchThread()
{
    DWORD newThreadId;
    //threadHandle = (void*) _beginthreadex (0, 0, &threadEntryProc, this, 0, &newThreadId);
    _threadHandle = (void*)fn_CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&threadEntryProc, this, 0, &newThreadId);
    _threadId = newThreadId;
}

void Thread::closeThreadHandle()
{
    fn_CloseHandle((HANDLE)_threadHandle);
    _threadId = 0;
    _threadHandle = 0;
}

void Thread::killThread()
{
    if (_threadHandle != 0) {
        fn_TerminateThread(_threadHandle, 0);
    }
}

uint32_t Thread::getCurrentThreadId()
{
    return (uint32_t)fn_GetCurrentThreadId();
}

bool Thread::setThreadPriority(void* handle, int priority)
{
    int pri = THREAD_PRIORITY_TIME_CRITICAL;

    if (priority < 1)       pri = THREAD_PRIORITY_IDLE;
    else if (priority < 2)  pri = THREAD_PRIORITY_LOWEST;
    else if (priority < 5)  pri = THREAD_PRIORITY_BELOW_NORMAL;
    else if (priority < 7)  pri = THREAD_PRIORITY_NORMAL;
    else if (priority < 9)  pri = THREAD_PRIORITY_ABOVE_NORMAL;
    else if (priority < 10) pri = THREAD_PRIORITY_HIGHEST;

    if (handle == 0)
        handle = fn_GetCurrentThread();

    return fn_SetThreadPriority(handle, pri) != FALSE;
}

void Thread::setCurrentThreadAffinityMask (const uint32_t affinityMask)
{
    fn_SetThreadAffinityMask(fn_GetCurrentThread(), affinityMask);
}

void SpinLock::enter() const throw()
{
    if (!tryEnter()) {
        for (int i = 20; --i >= 0; ) {
            if (tryEnter()) {
                return;
            }
        }

        while (!tryEnter()) {
            Thread::yield();
        }
    }
}

struct SleepEvent
{
    SleepEvent() :
    handle(fn_CreateEventW(0, 0, 0, 0))
    {
    }

    HANDLE handle;
};


static SleepEvent sleepEvent;

void __stdcall Thread::sleep(const int millisecs)
{
    if (millisecs >= 10) {
        fn_Sleep((DWORD)millisecs);
    }
    else {
        // unlike Sleep() this is guaranteed to return to the current thread after
        // the time expires, so we'll use this for short waits, which are more likely
        // to need to be accurate
        fn_WaitForSingleObject(sleepEvent.handle, (DWORD)millisecs);
    }
}

void Thread::yield()
{
    fn_Sleep(0);
}

}