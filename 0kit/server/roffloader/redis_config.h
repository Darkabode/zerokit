#ifndef __REDIS_CONFIG_H_
#define __REDIS_CONFIG_H_

#define _NATIVE_REDIS 1

#ifdef _NATIVE_REDIS
#include <hiredis/hiredis.h>
#else
#include "../../shared_code/hiredis/hiredis.h"
#endif // _NATIVE_REDIS

#define TS_STEP 300

#endif // __REDIS_CONFIG_H_
