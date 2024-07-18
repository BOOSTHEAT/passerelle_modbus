//=============================================
//
// Passerelle Modbus
//
// Environment variables | Description
// ------------------------------------- 
// LOGLEVEL              | 0 : Errors only 
//                       | 1 : Errors + warnings 
//                       | 2 : Errors + warnings + infos
//                       | 3 : All, including debug
// ------------------------------------- 
// SLAVE_SERIALDEVICE    | full path of the slave serial device
// ------------------------------------- 
// SLAVE_BAUDRATE        | baud rate of the serial connection
// ------------------------------------- 
// SLAVE_TIMEOUT         | timeout, in seconds, waiting for slave replies on serial connections
//
//=============================================


#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "modbus_compatibility.h"
#include "log.h"
#include "proxy.h"
#include "server.h"

#ifdef QT_QML_DEBUG
#define DEFAULT_LOG_LEVEL "4"
#else
#define DEFAULT_LOG_LEVEL "1"
#endif

#define CONTROL_ADDR 100
#define BLOCK_REMOTE_REQUEST_ADDR 0

static bool running;
static int delay = 0;

void sleep_ms(int ms){
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

static void handleRemoteClients(ModbusServer *server, uint8_t *data, int dataSize)
{
    if (data[14] == 1)
    {
        ModbusServer_disableRemoteConnections(server);
    }
    else
    {
        ModbusServer_enableRemoteConnections(server);
    }
    modbus_mapping_t *mb_map = modbus_mapping_new(0,0,1,0);
    if (mb_map == NULL)
    {
        LOG_ERROR("Cannot map inner modbus registers\n");
        return;
    }
    mb_map->tab_registers[0]=data[14];

    modbus_reply(server->context, data, dataSize, mb_map);
    modbus_mapping_free(mb_map);
    return;
}

void RequestHandler(ModbusServer *server, uint8_t *data, int dataSize, void *p)
{
    if (data[6] == CONTROL_ADDR && data[9] == BLOCK_REMOTE_REQUEST_ADDR)
    {
        handleRemoteClients(server,data, dataSize);
        return;
    }
    uint16_t transactionID = data[0]*256+data[1];
    modbus_set_slave(server->context, data[6]);
    ModbusProxy *proxy = (ModbusProxy *) p;

    if(delay>0) sleep_ms(delay);

    ModbusProxy_HandleRequest(proxy, server->context, transactionID, data+6, dataSize-6);
}

const char *getenvOrDefault(const char *key, const char *defaultValue)
{
    const char *value = getenv(key);
    if( value == NULL )
        value = defaultValue;
    LOG_DEBUG("> %s=%s\n", key, value);
    return value;
}

int main()
{
    logLevel = atoi(getenvOrDefault("LOGLEVEL",DEFAULT_LOG_LEVEL));
    delay = atoi(getenvOrDefault("DELAY","0"));
    LOG_INFO("Starting... LogLevel=%d debug=%d delay=%d\n", logLevel, IsLogDebug(), delay);
#ifdef TCPSLAVE
    const char *ipAddress = getenvOrDefault("TCPSLAVE_ADDRESS","127.0.0.1");
    int tcpPort = atoi(getenvOrDefault("TCPSLAVE_PORT","502"));
    LOG_INFO("Connecting to TCP slaves at address %s and port %d\n", ipAddress, tcpPort);
    modbus_t *slaves = modbus_new_tcp(ipAddress, tcpPort);
    MODBUS_EXECUTE("open slave connection",modbus_connect(slaves));
    ModbusProxy *proxy = ModbusProxy_create(slaves,6,0);
#else
    const char *serialDevice = getenvOrDefault("SLAVE_SERIALDEVICE","/dev/ttymxc1");
    int baudRate = atoi(getenvOrDefault("SLAVE_BAUDRATE","115200"));
    const char parity = getenvOrDefault("SLAVE_PARITY","N")[0];
    int data_bit = atoi(getenvOrDefault("SLAVE_DATA_BIT","8"));
    int stop_bit = atoi(getenvOrDefault("SLAVE_STOP_BIT","1"));


    LOG_INFO("Connecting to RTU slaves on serial device %s at baud rate %d, %c parity, %d data bit and %d stop bit\n", serialDevice, baudRate, parity, data_bit, stop_bit);
    modbus_t *slaves = modbus_new_rtu(serialDevice,baudRate,parity,data_bit,stop_bit);
    if(slaves == NULL)
      MODBUS_ERROR("create RTU slave");
    modbus_set_debug(slaves,IsLogDebug());
    int slaveTimeoutMs = atoi(getenvOrDefault("SLAVE_TIMEOUT","100"));
    modbus_set_response_timeout(slaves, slaveTimeoutMs/1000, (slaveTimeoutMs%1000)*1000);
    MODBUS_EXECUTE("open slave connection",modbus_connect(slaves));
    ModbusProxy *proxy = ModbusProxy_create(slaves,0,2);
#endif

    int masterTcpPort = atoi(getenvOrDefault("MASTER_TCPPORT","502"));
    ModbusServer *server = ModbusServer_create(masterTcpPort);
    ModbusServer_setHandler(server, RequestHandler, proxy);
    LOG_DEBUG("Entering main loop...\n");
    while(1)
        ModbusServer_run(server);
    LOG_INFO("Cleaning...\n");
    ModbusServer_delete(server);
    ModbusProxy_delete(proxy);
    modbus_close(slaves);
    modbus_free(slaves);
    return 0;
}
