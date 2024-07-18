#pragma once

#ifdef _WIN32
#include "modbus.h"
#else
#include <modbus/modbus.h>
#endif

#define MODBUS_ISSUE(type,txt,then) { type("Cannot %s - %s\n", txt, modbus_strerror(errno)); then; }

#define MODBUS_ERROR(txt) MODBUS_ISSUE(LOG_ERROR,txt,return errno)
#define MODBUS_EXECUTE(txt,op) ({int rc = (op); if(rc < 0) MODBUS_ERROR(txt); rc;})

#define MODBUS_WARNING(txt,fallback) MODBUS_ISSUE(LOG_WARNING,txt,fallback)
#define MODBUS_EXECUTE_WITH_FALLBACK(txt,op,fallback) ({int rc = (op); if(rc < 0) MODBUS_WARNING(txt,fallback); rc;})

