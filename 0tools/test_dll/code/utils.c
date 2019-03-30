#include <Windows.h>
#include "types.h"
#include "utils.h"

int utils_memcmp(const void* buf1, const void* buf2, size_t count)
{
    if (count == 0) {
        return 0;
    }

    while (--count && *(char*)buf1 == *(char*)buf2) {
        buf1 = (char*)buf1 + 1;
        buf2 = (char*)buf2 + 1;
    }

    return(*((uint8_t*)buf1) - *((uint8_t*)buf2));
}
