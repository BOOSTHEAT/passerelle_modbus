#pragma once

#include "modbus_compatibility.h"

#ifdef __cplusplus
extern "C"
{
#endif

int modbus_send_raw_request_with_tid(modbus_t *ctx, uint16_t tid, uint8_t *raw_req, int raw_req_length);
int modbus_get_slave_id(modbus_t *ctx);

#ifdef __cplusplus
}
#endif

