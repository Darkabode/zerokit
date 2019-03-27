#include "zgui.h"

namespace zgui
{

CriticalSection::CriticalSection() throw()
{
    ::InitializeCriticalSection(&_critSect);
}

CriticalSection::~CriticalSection() throw()
{
    ::DeleteCriticalSection(&_critSect);
}

void CriticalSection::enter() throw()
{
    ::EnterCriticalSection(&_critSect);
}

bool CriticalSection::tryEnter() throw()
{
    return (::TryEnterCriticalSection(&_critSect) != FALSE);
}

void CriticalSection::exit() throw()
{
    ::LeaveCriticalSection(&_critSect);
}

}
