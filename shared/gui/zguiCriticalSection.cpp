#include "zgui.h"

namespace zgui
{

CriticalSection::CriticalSection() throw()
{
    fn_InitializeCriticalSection(&_critSect);
}

CriticalSection::~CriticalSection() throw()
{
    fn_DeleteCriticalSection(&_critSect);
}

void CriticalSection::enter() throw()
{
    fn_EnterCriticalSection(&_critSect);
}

bool CriticalSection::tryEnter() throw()
{
    return (fn_TryEnterCriticalSection(&_critSect) != FALSE);
}

void CriticalSection::exit() throw()
{
    fn_LeaveCriticalSection(&_critSect);
}

}
