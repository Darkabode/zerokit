#ifndef __GLOBAL_CONFIG_H_
#define __GLOBAL_CONFIG_H_

typedef struct _global_config
{
	char* socketPath;		// Путь к файлу сокета
	char* syslogIdent;		// Идентификатор для syslog
	unsigned short logWarnings;	// Флаг записи в лог предупреждающих сообщений
	unsigned short logInfos;	// Флаг записи в лог информационных сообщений
	char* dbHost;			// Хост СУБД
	unsigned short dbPort;	// Порт СУБД
	char* dbName;			// Имя БД
	char* dbUser;			// Имя пользователя для доступа к БД
	char* dbPassword;		// Пароль пользователя
	size_t dbPoolSize;		// Размер пула соединений с базой
	size_t requestPoolSize;	// Размер пула памяти запросов
	size_t httpPostBodySize;	// размер пуля памяти для тела POST запроса
	char* geoipDataFile;		// Путь к файлу данных GeoIP
} global_config_t;


#endif // __GLOBAL_CONFIG_H_
