#include "zgui.h"

namespace zgui
{

WaitableEvent::WaitableEvent (const bool manualReset) throw() :
_hEvent(fn_CreateEventW(0, manualReset ? TRUE : FALSE, FALSE, 0))
{
}

WaitableEvent::~WaitableEvent() throw()
{
    fn_CloseHandle(_hEvent);
}

bool WaitableEvent::wait(const int timeOutMillisecs) const throw()
{
    return fn_WaitForSingleObject(_hEvent, (DWORD)timeOutMillisecs) == WAIT_OBJECT_0;
}

void WaitableEvent::signal() const throw()
{
    fn_SetEvent(_hEvent);
}

void WaitableEvent::reset() const throw()
{
    fn_ResetEvent(_hEvent);
}

}