#ifndef __DB_H_
#define __DB_H_

#include <mysql/mysql.h>

typedef struct _db_connection
{
	MYSQL mysql;
	int connected;
} db_connection_t, *pdb_connection_t;

int db_init(size_t poolSize);
void db_done();

MYSQL* db_get_mysql();
void db_release_mysql(MYSQL* pConn);

int db_connect(MYSQL* pMysql);
int db_query(MYSQL* pMysql, const char* query);

#endif // __DB_H_
