#pragma once

#include <stdio.h>
#include <stdbool.h>

extern int logLevel;

#ifdef __cplusplus
extern "C"
{
#endif

void Logger(int level, FILE *stream, const char *format, ...);
bool IsLogDebug();

#ifdef __cplusplus
}
#endif

#define LOG_ERROR(format,...)   Logger(0, stderr, format, ##__VA_ARGS__)
#define LOG_WARNING(format,...) Logger(1, stderr, format, ##__VA_ARGS__)
#define LOG_INFO(format,...)    Logger(2, stdout, format, ##__VA_ARGS__)
#define LOG_DEBUG(format,...)   Logger(3, stdout, format, ##__VA_ARGS__)


