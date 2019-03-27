#ifndef __LOGGER_H_
#define __LOGGER_H_

int logger_init(int isDaemon);
void logger_done();

extern void (*logger_error)(const char* fmt, ...);
extern void (*logger_warn)(const char* fmt, ...);
extern void (*logger_info)(const char* fmt, ...);

#ifdef LOG_PERF

extern void (*logger_perf)(const char* fmt, ...);

#define USE_PERF unsigned long tsc
#define BEGIN_PERF tsc = hardclock()
#define END_PERF(msg) logger_perf("%s: %lu ms", msg, (hardclock() - tsc) / 1000)

#else

#define USE_PERF
#define BEGIN_PERF
#define END_PERF(msg)

#endif // LOG_PERF

#endif // __LOGGER_H_
