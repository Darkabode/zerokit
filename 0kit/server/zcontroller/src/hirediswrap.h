#ifndef __HIREDISWRAP_H_
#define __HIREDISWRAP_H_

#include "Poco/Logger.h"

/** ћетоды в которых выполн€ютс€ запросы к редису не €вл€ютс€ потокобезопасными, поэтому ннеобходимо пользоватьс€ функци€ми lock/unlock. */
class RedisWrapper
{
public:
    RedisWrapper(const std::string& host, uint16_t port, Poco::Logger& logger);
    ~RedisWrapper();
    int connect();
    void checkConnection();

    int getBotInternalId(char* query, const char* const uniqid, Poco::UInt64& internalId);
    int getBotDTF(char* query, Poco::UInt64 internalId, Poco::UInt32& dtf);
    int getNextBotId(Poco::UInt64& insideId);
    void lock() { _mutex.lock(); }
    void unlock() { _mutex.unlock(); }
    int execureCommand(const char* query);

    int collectBotStats(char* query, int isNewBot, uint64_t insideId, const char* uniqid, uint32_t affId, uint16_t subId, uint8_t ver, uint16_t osVer, uint16_t osLang, uint64_t hips, uint32_t ip, char* countryCode);
    int getBotInfo(char* query, uint64_t insideId, uint8_t* pVer, uint32_t* pAffId, uint32_t* pSubId, uint16_t* pOsVer, uint16_t* pOsLang, uint64_t* pHips, char* cc);

private:
    void logError(const char* query, int err);
    Poco::Logger& _logger;
    std::string _host;
    uint16_t _port;
    struct redisContext* _redisCtx;
    struct timeval _timeout;
    Poco::FastMutex _mutex;
};

#endif // __HIREDISWRAP_H_
