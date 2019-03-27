#include "zgui.h"

namespace zgui
{

WaitableEvent::WaitableEvent (const bool manualReset) throw() :
_internal(::CreateEvent(0, manualReset ? TRUE : FALSE, FALSE, 0))
{
}

WaitableEvent::~WaitableEvent() throw()
{
    ::CloseHandle(_internal);
}

bool WaitableEvent::wait(const int timeOutMillisecs) const throw()
{
    return ::WaitForSingleObject(_internal, (DWORD)timeOutMillisecs) == WAIT_OBJECT_0;
}

void WaitableEvent::signal() const throw()
{
    ::SetEvent(_internal);
}

void WaitableEvent::reset() const throw()
{
    ::ResetEvent(_internal);
}

}