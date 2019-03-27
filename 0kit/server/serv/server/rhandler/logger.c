#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <pthread.h>
#if LOG_IN_SYSLOG
#include <time.h>
#endif // LOG_IN_SYSLOG

#include "../../shared_code/types.h"

#include "globalconfig.h"
#include "logger.h"

#define SYSLOG_BUF_SIZE 1024

extern global_config_t gGlobalConfig;

static pthread_mutex_t gLoggerMutex = PTHREAD_MUTEX_INITIALIZER;
static char* logBuff = NULL;
static char logFilePath[256];

void (*logger_error)(const char* fmt, ...) = NULL;
void (*logger_warn)(const char* fmt, ...) = NULL;
void (*logger_info)(const char* fmt, ...) = NULL;

#ifdef LOG_PERF
void (*logger_perf)(const char* fmt, ...) = NULL;
#endif // LOG_PERF

void syslog_logger_error(const char* fmt, ...)
{
	va_list args;

	pthread_mutex_lock(&gLoggerMutex);
	va_start(args, fmt);
	vsnprintf(logBuff, SYSLOG_BUF_SIZE, fmt, args);
	va_end(args);

	syslog(LOG_LOCAL0 | LOG_ERR, "%.1024s", logBuff);

	pthread_mutex_unlock(&gLoggerMutex);
}

void syslog_logger_warn(const char* fmt, ...)
{
	va_list args;

	if (gGlobalConfig.logWarnings) {
		pthread_mutex_lock(&gLoggerMutex);
		va_start(args, fmt);
		vsnprintf(logBuff, SYSLOG_BUF_SIZE, fmt, args);
		va_end(args);

		syslog(LOG_LOCAL0 | LOG_WARNING, "%.1024s", logBuff);

		pthread_mutex_unlock(&gLoggerMutex);
	}
}

void syslog_logger_info(const char* fmt, ...)
{
	va_list args;

	if (gGlobalConfig.logInfos) {
		pthread_mutex_lock(&gLoggerMutex);
		va_start(args, fmt);
		vsnprintf(logBuff, SYSLOG_BUF_SIZE, fmt, args);
		va_end(args);

		syslog(LOG_LOCAL0 | LOG_INFO, "%.1024s", logBuff);

		pthread_mutex_unlock(&gLoggerMutex);
	}
}

#ifdef LOG_PERF

void syslog_logger_perf(const char* fmt, ...)
{
	va_list args;

	pthread_mutex_lock(&gLoggerMutex);
	va_start(args, fmt);
	vsnprintf(logBuff, SYSLOG_BUF_SIZE, fmt, args);
	va_end(args);

	syslog(LOG_LOCAL0 | LOG_INFO, "%.1024s", logBuff);

	pthread_mutex_unlock(&gLoggerMutex);
}


#endif // LOG_PERF

void log_in_file(const char* prefix, const char* logBuff)
{
	FILE* pLogFile;
	time_t now;
	struct tm* ts;

	pLogFile = fopen(logFilePath, "a");
	now = time(NULL);
	ts = localtime(&now);
	fprintf(pLogFile, "%s: %02d/%02d/%04d, %02d:%02d:%02d\t%s\n", prefix, ts->tm_mday, ts->tm_mon, ts->tm_year, ts->tm_hour, ts->tm_min, ts->tm_sec, logBuff);
	fclose(pLogFile);
}

void file_logger_error(const char* fmt, ...)
{
	va_list args;

	pthread_mutex_lock(&gLoggerMutex);
	va_start(args, fmt);
	vsnprintf(logBuff, SYSLOG_BUF_SIZE, fmt, args);
	va_end(args);

	log_in_file("Error", logBuff);

	pthread_mutex_unlock(&gLoggerMutex);
}

void file_logger_warn(const char* fmt, ...)
{
	va_list args;

	if (gGlobalConfig.logWarnings) {
		pthread_mutex_lock(&gLoggerMutex);
		va_start(args, fmt);
		vsnprintf(logBuff, SYSLOG_BUF_SIZE, fmt, args);
		va_end(args);

		log_in_file("Warning", logBuff);

		pthread_mutex_unlock(&gLoggerMutex);
	}
}

void file_logger_info(const char* fmt, ...)
{
	va_list args;

	if (gGlobalConfig.logInfos) {
		pthread_mutex_lock(&gLoggerMutex);
		va_start(args, fmt);
		vsnprintf(logBuff, SYSLOG_BUF_SIZE, fmt, args);
		va_end(args);

		log_in_file("Info", logBuff);

		pthread_mutex_unlock(&gLoggerMutex);
	}
}

#ifdef LOG_PERF

void file_logger_perf(const char* fmt, ...)
{
	va_list args;

	pthread_mutex_lock(&gLoggerMutex);
	va_start(args, fmt);
	vsnprintf(logBuff, SYSLOG_BUF_SIZE, fmt, args);
	va_end(args);

	log_in_file("Perf", logBuff);

	pthread_mutex_unlock(&gLoggerMutex);
}

#endif // LOG_PERF

int logger_init(int isDaemon)
{
	logBuff = malloc(SYSLOG_BUF_SIZE + 1);

	if (logBuff == NULL)
		return 1;

	if (isDaemon) {
		setlogmask(LOG_UPTO(LOG_DEBUG));
		openlog(gGlobalConfig.syslogIdent, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);

		logger_error = syslog_logger_error;
		logger_warn = syslog_logger_warn;
		logger_info = syslog_logger_info;
#ifdef LOG_PERF
		logger_perf = syslog_logger_perf;
#endif // LOG_PERF
	}
	else {
		time_t now;
		struct tm* ts;

		now = time(NULL);
		ts = localtime(&now);

		logFilePath[0] = 0;

		sprintf(logFilePath, "/tmp/handler_%d%d%d%d%d%d.log", ts->tm_mday, ts->tm_mon, ts->tm_year, ts->tm_hour, ts->tm_min, ts->tm_sec);

		logger_error = file_logger_error;
		logger_warn = file_logger_warn;
		logger_info = file_logger_info;
#ifdef LOG_PERF
		logger_perf = file_logger_perf;
#endif // LOG_PERF
		printf("Log file path: %s\n", logFilePath);
	}

	return 0;
}

void logger_done()
{
#if LOG_IN_SYSLOG
	closelog();
#endif // LOG_IN_SYSLOG
	if (logBuff)
		free(logBuff);
}
