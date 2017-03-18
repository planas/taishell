#ifndef __TAISHELL_LOGGER_H__
#define __TAISHELL_LOGGER_H__

#define LOG_LEVEL_DEBUG  'D'
#define LOG_LEVEL_INFO   'I'
#define LOG_LEVEL_NOTICE 'N'
#define LOG_LEVEL_WARN   'W'
#define LOG_LEVEL_ERROR  'E'

#ifdef __VITA_KERNEL__
#define LOG_FUNC ktshLog
#else
#include <psp2/kernel/clib.h>
char tsh_log_buff[512];
#define LOG_FUNC(level, fmt, ...) \
  do { \
    sceClibSnprintf(tsh_log_buff, 511, fmt, ##__VA_ARGS__); \
    tshLog(level, tsh_log_buff); \
  } while(0)
#endif

#define LOG_D(fmt,...) LOG_FUNC(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_I(fmt,...) LOG_FUNC(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_N(fmt,...) LOG_FUNC(LOG_LEVEL_NOTICE, fmt, ##__VA_ARGS__)
#define LOG_W(fmt,...) LOG_FUNC(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_E(fmt,...) LOG_FUNC(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

int ktshLog(const char level, const char *msg, ...) __attribute__ ((format (printf, 2, 3)));
int tshLog(const char level, const char *msg);

#endif