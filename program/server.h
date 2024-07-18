#pragma once

#include "modbus_compatibility.h"
#include <sys/select.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct SModbusServer;

typedef void (*ModbusServerRequestHandler)(struct SModbusServer*,uint8_t*,int,void*);

typedef struct SModbusServer
{
    modbus_t *context;
    int serverSocket;
    ModbusServerRequestHandler handler;
    void *userData;
    fd_set sockets;
    int maxSocket;
    bool remoteBlocked;
} ModbusServer;

ModbusServer *ModbusServer_create(int);
void ModbusServer_setHandler(ModbusServer *,ModbusServerRequestHandler, void *);
void ModbusServer_run(ModbusServer *);
void ModbusServer_delete(ModbusServer *);
void ModbusServer_disableRemoteConnections(ModbusServer *);
void ModbusServer_enableRemoteConnections(ModbusServer *);

#ifdef __cplusplus
}
#endif
