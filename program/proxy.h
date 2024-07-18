#pragma once

#include "modbus_compatibility.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct SModbusProxy
{
    modbus_t *target;
    uint8_t szHeader;
    uint8_t szFooter;
} ModbusProxy;

ModbusProxy *ModbusProxy_create(modbus_t *target, uint8_t szHeader, uint8_t szFooter);
int ModbusProxy_HandleRequest(ModbusProxy *proxy, modbus_t *origin, uint16_t tid, const uint8_t *data, int dataSize);
void ModbusProxy_delete(ModbusProxy *);

#ifdef __cplusplus
}
#endif
