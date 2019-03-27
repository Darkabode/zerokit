#ifndef __GLOBALS_H_
#define __GLOBALS_H_

#include "Poco/MemoryPool.h"
#include "Poco/File.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Util/LayeredConfiguration.h"

#ifdef _WIN32
extern HANDLE gHeap;

#define SYS_ALLOCATOR(sz) HeapAlloc(gHeap, 0, sz)
#define SYS_ALLOCATORZ(sz) HeapAlloc(gHeap, HEAP_ZERO_MEMORY, sz)
#define SYS_DEALLOCATOR(ptr) HeapFree(gHeap, 0, ptr)
#define SYS_REALLOCATOR(ptr, newSz) HeapReAlloc(gHeap, 0, ptr, newSz)

#include <intrin.h>
#endif // _WIN32

#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/bignum.h"
#include "../../../shared_code/bn_mul.h"
#include "../../../shared_code/rsa.h"
#include "../../../shared_code/crc64.h"

#include "GeoIP.h"
#include "hirediswrap.h"

class Globals
{
public:
    Globals(Poco::Util::LayeredConfiguration& config);
    ~Globals();

    const char* geoip_get_country(const char* ip);
    void readFile(Poco::File& file, char* data);

    Poco::UInt32 _sid;
    int _poolMinSize;
    int _poolMaxSize;
    Poco::MemoryPool gPostBodyPool;
    Poco::MemoryPool gShaHashPool;
    Poco::MemoryPool gQueryPool;
    Poco::MemoryPool gBotInfoPool;
    rsa_context_t gRsaContext;
    GeoIP* pGip;
    Poco::Data::SessionPool _mysqlPool;
    std::string _bundlesPathPrefix;
    std::string _redisHost;
    Poco::UInt16 _redisPort;
    Poco::SharedPtr<RedisWrapper> _pRedisSysInfo;
    Poco::SharedPtr<RedisWrapper> _pRedisTasksRequest;
    Poco::SharedPtr<RedisWrapper> _pRedisTasksReport;
};

#endif // __GLOBALS_H_
