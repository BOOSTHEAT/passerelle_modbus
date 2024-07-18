#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "log.h"
#include "proxy.h"
#include "raw_request_with_tid.h"

ModbusProxy *ModbusProxy_create(modbus_t *target, uint8_t szHeader, uint8_t szFooter)
{
    ModbusProxy *proxy = malloc(sizeof(ModbusProxy));
    proxy->target = target;
    proxy->szHeader = szHeader;
    proxy->szFooter = szFooter;
    return proxy;
}

int ModbusProxy_HandleRequest(ModbusProxy *proxy, modbus_t *origin, uint16_t tid, const uint8_t *data, int szData)
{
    int slave = modbus_get_slave_id(origin);
    LOG_DEBUG("Proxy request to slave %d for tid %d\n",slave,tid);
    modbus_set_slave(proxy->target, slave);
    int sent_bytes_to_slave = MODBUS_EXECUTE("send request to slave",modbus_send_raw_request(proxy->target, (uint8_t *) data, szData));
    LOG_DEBUG("Proxy successfully sent request to slave with %d bytes for tid %d\n",sent_bytes_to_slave,tid);
#if defined(_WIN32)
    modbus_flush(proxy->target);
#endif
    uint8_t response[MODBUS_MAX_PDU_LENGTH];
    int received_bytes_from_slave = MODBUS_EXECUTE_WITH_FALLBACK(
        "receive reply from slave",
        modbus_receive_confirmation(proxy->target, response),
        MODBUS_EXECUTE("reply exception",modbus_reply_exception(origin, data, MODBUS_EXCEPTION_GATEWAY_TARGET))
    );
    LOG_DEBUG("Proxy received reply from slave with %d bytes for tid %d\n",received_bytes_from_slave,tid);
    int replied_bytes_to_master = MODBUS_EXECUTE("reply to master",modbus_send_raw_request_with_tid(origin, tid, response+proxy->szHeader, received_bytes_from_slave-proxy->szHeader-proxy->szFooter));
    LOG_DEBUG("Proxy successfully sent reply with %d bytes for tid %d\n",replied_bytes_to_master,tid);
    return 0;
}

void ModbusProxy_delete(ModbusProxy *proxy)
{
    free(proxy);
}
