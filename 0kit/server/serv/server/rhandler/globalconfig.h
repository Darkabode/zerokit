#ifndef __GLOBAL_CONFIG_H_
#define __GLOBAL_CONFIG_H_

typedef struct _global_config
{
	char* socketPath;		// Путь к файлу сокета
	char* syslogIdent;		// Идентификатор для syslog
	char* filesPathPrefix;      // 
	uint16_t logWarnings;       // Флаг записи в лог предупреждающих сообщений
	uint16_t logInfos;          // Флаг записи в лог информационных сообщений
	char* dbHost;			// Хост СУБД
	uint16_t dbPort;            // Порт СУБД
	char* dbName;			// Имя БД
	char* dbUser;			// Имя пользователя для доступа к БД
	char* dbPassword;		// Пароль пользователя
	size_t dbPoolSize;		// Размер пула соединений с базой
	size_t minPoolSize;		// Минимальный размер пулов.
	size_t maxPoolSize;		// Максимальный размер пулов.
	size_t httpPostBodySize;	// размер пуля памяти для тела POST запроса
	uint16_t tasksPerConn;      // Максимальное количество выдываемых заданий за одно соединение.
	char* geoipDataFile;		// Путь к файлу данных GeoIP
} global_config_t;

#endif // __GLOBAL_CONFIG_H_
