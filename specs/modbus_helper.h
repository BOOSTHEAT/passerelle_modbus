#pragma once

#include "modbus.h"

#ifdef __cplusplus
#include <vector>
#include <functional>

class ModbusQuery;

typedef void (*modbus_helper_receive_IT)(void*,const uint8_t *, int);
typedef void (*modbus_helper_reply_IT)(void*,const ModbusQuery &);


modbus_t* modbus_new_helper(void *,modbus_helper_receive_IT,modbus_helper_reply_IT);

bool modbus_has_response(modbus_t*);


class ModbusQuery
{
public:
    ModbusQuery(modbus_t *ctx, uint8_t *d, uint16_t s);
    void ReplyRegisters(const std::vector<uint16_t> &reply) const;
    void ReplyException(unsigned int code) const;
    uint8_t Slave() const { return data[0]; }
    uint8_t Function() const { return data[1]; }
    unsigned int Address() const { return (data[2] << 8) + data[3]; }

private:
    modbus_t *context;
    uint8_t *data;
    uint16_t size;
};

std::vector<uint8_t> modbus_create_request(std::function<void(modbus_t*)> action);
std::vector<uint8_t> modbus_create_reply(const uint8_t *req, int req_length, modbus_mapping_t *mb_mapping);

extern "C"
{
#endif

void modbus_transmit(modbus_t*, uint8_t *, uint16_t);
void modbus_server_receive(modbus_t*, uint8_t *, uint16_t);
int modbus_fill_reply(const uint8_t *req, int req_length, modbus_mapping_t *mb_mapping, uint8_t *reply);

#ifdef __cplusplus
}
#endif
