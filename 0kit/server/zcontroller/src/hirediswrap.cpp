#include "hiredis/config.h"
#include "hiredis/async.h"
#include "hiredis/net.h"
#include "hiredis/sds.h"
#include "hiredis/hiredis.h"

#include "hiredis/async.c"
#include "hiredis/net.c"
#include "hiredis/sds.c"
#include "hiredis/hiredis.c"

#include "hirediswrap.h"

#include "Poco/ScopedLock.h"

RedisWrapper::RedisWrapper(const std::string& host, uint16_t port, Poco::Logger& logger) :
_host(host),
_port(port),
_logger(logger),
_redisCtx(0)
{
    _timeout.tv_sec = 3;
    _timeout.tv_usec = 0;
}

RedisWrapper::~RedisWrapper()
{

}

int RedisWrapper::connect()
{
    int err;
    _redisCtx = redisConnectWithTimeout(_host.data(), _port, _timeout);
    err = _redisCtx->err;
    if (err) {
        redisFree(_redisCtx);
        _redisCtx = 0;
        _logger.error("Cannot connect to redis with error " + Poco::NumberFormatter::format(err));
    }

    return err;
}

void RedisWrapper::checkConnection()
{
    static const char query[8] = "PING";
    Poco::ScopedLock<Poco::FastMutex> lock(_mutex);

    if (execureCommand(query) != ERR_OK) {
        redisFree(_redisCtx);
        connect();
    }
}

int RedisWrapper::getBotInternalId(char* query, const char* const uniqid, Poco::UInt64& internalId)
{
    int err = ERR_OK;
    struct redisReply* pRedisReply = 0;

    if (_redisCtx == 0) {
        _logger.error("No connection with redis");
        return ERR_REDIS_CONNECT;
    }

    do {
        snprintf(query, 1024, "ZSCORE zids %.64s", uniqid);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            err = ERR_REDIS;
            break;
        }
        if (pRedisReply->type == REDIS_REPLY_NIL) {
            _logger.information("Bot " + std::string(uniqid) + " not exists");
            err = ERR_BOT_NOT_EXISTS;
            break;
        }
        internalId = Poco::NumberParser::parseUnsigned64(pRedisReply->str);
    } while (0);

    if (pRedisReply != 0) {
        freeReplyObject(pRedisReply);
    }

    return err;
}

int RedisWrapper::getBotDTF(char* query, Poco::UInt64 internalId, Poco::UInt32& dtf)
{
    int err = ERR_OK;
    struct redisReply* pRedisReply = 0;

    if (_redisCtx == 0) {
        _logger.error("No connection with redis");
        return ERR_REDIS_CONNECT;
    }

    do {
        snprintf(query, 1024, "HGET dtf %llu", internalId);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            err = ERR_REDIS;            
            break;
        }
        dtf = Poco::NumberParser::parseUnsigned(pRedisReply->str);
    } while (0);

    if (pRedisReply != 0) {
        freeReplyObject(pRedisReply);
    }

    return err;
}

int RedisWrapper::getNextBotId(Poco::UInt64& insideId)
{
    int err = ERR_OK;
    struct redisReply* pRedisReply = 0;

    if (_redisCtx == 0) {
        _logger.error("No connection with redis");
        return ERR_REDIS_CONNECT;
    }

    do {
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, "INCR bot_next_id");
        if (pRedisReply == 0) {
            logError("INCR bot_next_id", _redisCtx->err);
            err = ERR_REDIS;
            break;
        }
        insideId = (Poco::UInt64)pRedisReply->integer;
    } while (0);

    if (pRedisReply != 0) {
        freeReplyObject(pRedisReply);
    }

    return err;
}

int RedisWrapper::execureCommand(const char* query)
{
    int err = ERR_OK;
    struct redisReply* pRedisReply = 0;

    if (_redisCtx == 0) {
        _logger.error("No connection with redis");
        return ERR_REDIS_CONNECT;
    }

    do {
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            err = ERR_REDIS;
            break;
        }
    } while (0);

    if (pRedisReply != 0) {
        freeReplyObject(pRedisReply);
    }

    return err;
}

int RedisWrapper::collectBotStats(char* query, int isNewBot, uint64_t insideId, const char* uniqid, uint32_t affId, uint16_t subId, uint8_t ver, uint16_t osVer, uint16_t osLang, uint64_t hips, uint32_t ip, char* countryCode)
{
    int err = ERR_OK;
    uint32_t currTimestamp;
    uint32_t ts, prevTs = 0;
    struct redisReply* pRedisReply = 0;

    if (_redisCtx == 0) {
        _logger.error("No connection with redis");
        return ERR_REDIS_CONNECT;
    }

    do {
        currTimestamp = (uint32_t)time(NULL);
        ts = currTimestamp / TS_STEP;

        snprintf(query, 1024, "LINDEX ltss -1");
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            err = ERR_REDIS;
            break;
        }

        if (pRedisReply->type == REDIS_REPLY_NIL || (pRedisReply->type == REDIS_REPLY_STRING && (prevTs = Poco::NumberParser::parseUnsigned(pRedisReply->str)) < ts)) {
            freeReplyObject(pRedisReply);
            snprintf(query, 1024, "RPUSH ltss %u", ts);
            pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
            if (pRedisReply == 0) {
                logError(query, _redisCtx->err);
                err = ERR_REDIS;
                break;
            }
        }

        if (isNewBot) {
            snprintf(query, 1024, "RPUSH bh:%u %llu:%u:%.64s:%u:%u:%u:%u:%u:%u:%u:%llu:%.2s:%u", ts, insideId, isNewBot, uniqid, currTimestamp, currTimestamp, affId, subId, ver, osVer, osLang, hips, countryCode, ip);
        }
        else {
            snprintf(query, 1024, "HGET dtf %llu", insideId);
            pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
            if (pRedisReply == 0) {
                logError(query, _redisCtx->err);
                err = ERR_REDIS;
                break;
            }

            snprintf(query, 1024, "RPUSH bh:%u %llu:%u:%.64s:%u:%u:%u:%u:%u:%u:%u:%llu:%.2s:%u", ts, insideId, isNewBot, uniqid, Poco::NumberParser::parseUnsigned(pRedisReply->str), currTimestamp, affId, subId, ver, osVer, osLang, hips, countryCode, ip);
            freeReplyObject(pRedisReply);
        }

        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            err = ERR_REDIS;
        }
    } while (0);

    if (err != ERR_OK) {
        _logger.error("Cannot execute redis command '" + std::string(query) + "'");
    }

    if (pRedisReply != 0) {
        freeReplyObject(pRedisReply);
    }

    return err;
}

int RedisWrapper::getBotInfo(char* query, uint64_t insideId, uint8_t* pVer, uint32_t* pAffId, uint32_t* pSubId, uint16_t* pOsVer, uint16_t* pOsLang, uint64_t* pHips, char* cc)
{
    int err = ERR_REDIS;
    struct redisReply* pRedisReply;

    if (_redisCtx == 0) {
        _logger.error("No connection with redis");
        return ERR_REDIS_CONNECT;
    }

    do {
        snprintf(query, 1024, "HGET affids %llu", insideId);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            break;
        }
        if (pRedisReply->type != REDIS_REPLY_NIL) {
            *pAffId = Poco::NumberParser::parseUnsigned(pRedisReply->str);
        }
        freeReplyObject(pRedisReply);

        snprintf(query, 1024, "HGET subids %llu", insideId);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            break;
        }
        if (pRedisReply->type != REDIS_REPLY_NIL) {
            *pSubId = Poco::NumberParser::parseUnsigned(pRedisReply->str);
        }
        freeReplyObject(pRedisReply);

        snprintf(query, 1024, "HGET vers %llu", insideId);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            break;
        }
        if (pRedisReply->type != REDIS_REPLY_NIL) {
            *pVer = (uint8_t)atoi(pRedisReply->str);
        }
        freeReplyObject(pRedisReply);

        snprintf(query, 1024, "HGET osvers %llu", insideId);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            break;
        }
        if (pRedisReply->type != REDIS_REPLY_NIL) {
            *pOsVer = (uint16_t)atoi(pRedisReply->str);
        }
        freeReplyObject(pRedisReply);

        snprintf(query, 1024, "HGET oslngs %llu", insideId);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            break;
        }
        if (pRedisReply->type != REDIS_REPLY_NIL) {
            *pOsLang = (uint16_t)atoi(pRedisReply->str);
        }
        freeReplyObject(pRedisReply);

        snprintf(query, 1024, "HGET hipses %llu", insideId);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            break;
        }
        if (pRedisReply->type != REDIS_REPLY_NIL) {
            *pHips = Poco::NumberParser::parseUnsigned64(pRedisReply->str);
        }
        freeReplyObject(pRedisReply);

        snprintf(query, 1024, "HGET geos %llu", insideId);
        pRedisReply = (struct redisReply*)redisCommand(_redisCtx, query);
        if (pRedisReply == 0) {
            logError(query, _redisCtx->err);
            break;
        }
        if (pRedisReply->type != REDIS_REPLY_NIL) {
            strncpy(cc, pRedisReply->str, 2);	
        }		
        freeReplyObject(pRedisReply);

        err = ERR_OK;
    } while (0);

    if (err != ERR_OK) {
        _logger.error("Cannot execute redis command '" + std::string(query) + "'");
    }

    return err;
}

void RedisWrapper::logError(const char* query, int err)
{
    _logger.error("Cannot execute redis command '" + std::string(query) + "' with error " + Poco::NumberFormatter::format(err));
}
