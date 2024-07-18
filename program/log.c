#include "log.h"

#include <stdarg.h>

int logLevel;

void Logger(int level, FILE *stream, const char *format, ...)
{
  if(level > logLevel)
    return;
  va_list args;
  va_start (args, format);
  vfprintf (stream, format, args);
  va_end (args);
}

bool IsLogDebug()
{
  return logLevel >= 4;
}
