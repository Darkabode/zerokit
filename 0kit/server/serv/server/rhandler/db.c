#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <mysql/errmsg.h>

#include "../../shared_code/types.h"

#include "globalconfig.h"
#include "errors.h"
#include "logger.h"
#include "db.h"
#include "mempool.h"


extern global_config_t gGlobalConfig;

static memory_pool_t gDbPool;

int db_init(size_t minPoolSize, size_t maxPoolSize)
{
	if (mempool_init(&gDbPool, sizeof(db_connection_t), minPoolSize, maxPoolSize))
		return ERR_MEMPOOL;

	if (mysql_library_init(0, 0, 0) != 0) {
		logger_error("Can't initialize MySql library");
		return ERR_MYSQL;
	}

	logger_info("DB has successfully initialized");
	return 0;
}

void db_done()
{	
//	int i;

//	for (i = 0; i < gDbPool.size; ++i) {
//		mysql_close(&gDbPool.pool[i].mysql);
//	}

	mempool_done(&gDbPool);
	
	mysql_library_end();

	logger_info("DB has successfully uninitialized");
}

MYSQL* db_get_mysql()
{
	MYSQL* pMysql = NULL;
	pdb_connection_t pConnection;

	pConnection = mempool_get(&gDbPool);

	if (pConnection != NULL) {
		if (pConnection->connected == 0) {
			mysql_init(&pConnection->mysql);
			if (db_connect(&pConnection->mysql) == 0) {
				pConnection->connected = 1;
				pMysql = &pConnection->mysql;
			}
			else {
				mysql_close(&pConnection->mysql);
				mempool_release(&gDbPool, pConnection);
				logger_warn("Can't connect to DB");
				return NULL;
			}
		}
		else {
			pMysql = &pConnection->mysql;
		}

		logger_info("DB connection has gained from pool");
	}
	else {
		logger_warn("Can't obtain DB connection from pool");
	}

	return pMysql;
}

void db_release_mysql(MYSQL* mysql)
{
	mempool_release(&gDbPool, mysql);
}

int db_connect(MYSQL* mysql)
{
	my_bool reconnect = 1;
	mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);
	if (mysql_real_connect(mysql, gGlobalConfig.dbHost, gGlobalConfig.dbUser, gGlobalConfig.dbPassword, gGlobalConfig.dbName, gGlobalConfig.dbPort, 0, 0) == NULL) {
		logger_error("Can't connect to database %s (%s:%d). MYSQL error: %s", gGlobalConfig.dbName, gGlobalConfig.dbHost, gGlobalConfig.dbPort, mysql_error(mysql));
		return 1;
	}
	return 0;
}

int db_query(MYSQL* mysql, const char* query)
{
	int res;
	do {
		res = mysql_real_query(mysql, query, (unsigned long)strlen(query));
		if (res == 0)
			return 0;
		break;	
	} while (1);
	
	logger_error("mysql_real_query('%s') error. MySql error: %s", query, mysql_error(mysql));
	return mysql_errno(mysql);
}
