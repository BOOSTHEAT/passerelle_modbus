#include "modbus_helper.h"

#include "modbus-private.h"

#include <vector>

#define MODBUS_BACKEND_TYPE      1
#define MODBUS_HEADER_LENGTH      1
#define MODBUS_CHECKSUM_LENGTH    2

modbus_backend_t *modbus_create_fake_backend()
{
    modbus_t *mbrtu = modbus_new_rtu("x", 1, 'N', 0, 0);
    modbus_backend_t *fake_backend = new modbus_backend_t {
        MODBUS_BACKEND_TYPE,
        MODBUS_HEADER_LENGTH,
        MODBUS_CHECKSUM_LENGTH,
        MODBUS_MAX_ADU_LENGTH,
        mbrtu->backend->set_slave,
        mbrtu->backend->build_request_basis,
        mbrtu->backend->build_response_basis,
        mbrtu->backend->prepare_response_tid,
        mbrtu->backend->send_msg_pre,
        mbrtu->backend->send,
        mbrtu->backend->receive,
        mbrtu->backend->recv,
        mbrtu->backend->check_integrity,
        mbrtu->backend->pre_check_confirmation,
        mbrtu->backend->connect,
        mbrtu->backend->close,
        mbrtu->backend->flush,
        mbrtu->backend->select,
        mbrtu->backend->free
    };
    modbus_free(mbrtu);
    return fake_backend;
}

struct helper_data
{
    void *user_data;
    modbus_helper_receive_IT recv_handler;
    std::vector<uint8_t> transmitted_data;
    std::vector<uint8_t>::iterator cursor;
    modbus_helper_reply_IT reply_handler;
};

static int modbus_helper_connect(modbus_t *ctx)
{
    return 0;
}

static void modbus_helper_close(modbus_t *ctx)
{
}

static int modbus_helper_select(modbus_t *ctx, fd_set *rset,
                              struct timeval *tv, int length_to_read)
{
    return 0;
}

static ssize_t modbus_helper_send(modbus_t *ctx, const uint8_t *req, int req_length)
{
    std::vector<uint8_t> received;
    received.assign(req, req + req_length);
    helper_data *hd = (helper_data *)ctx->backend_data;
    hd->recv_handler(hd->user_data,received.data(),received.size());
    return req_length;
}

void modbus_transmit(modbus_t *ctx, uint8_t *data, uint16_t size)
{
    helper_data *hd = (helper_data *)ctx->backend_data;
    hd->transmitted_data.assign(data, data+size);
    hd->cursor = hd->transmitted_data.begin();
}

bool modbus_has_response(modbus_t *ctx)
{
    helper_data *hd = (helper_data *)ctx->backend_data;
    return hd->cursor != hd->transmitted_data.end();
}


static ssize_t modbus_helper_recv(modbus_t *ctx, uint8_t *rsp, int rsp_length)
{
    helper_data *hd = (helper_data *)ctx->backend_data;
    ssize_t n = 0;
    uint8_t *rspEnd = rsp+rsp_length;
    while((rsp < rspEnd) && (hd->cursor != hd->transmitted_data.end()))
    {
        *rsp++ = *(hd->cursor++);
        n++;
    }
    return n;
}


ModbusQuery::ModbusQuery(modbus_t *ctx, uint8_t *d, uint16_t s)
{
    context = ctx;
    data = d;
    size = s;
}

void ModbusQuery::ReplyRegisters(const std::vector<uint16_t> &reply) const
{
    modbus_mapping_t *mm = modbus_mapping_new_start_address(0, 0, 0, 0, Address(), reply.size()+1, 0, 0);
    std::copy(std::begin(reply), std::end(reply), mm->tab_registers);
    modbus_reply(context, data, size, mm);
    modbus_mapping_free(mm);
}

void ModbusQuery::ReplyException(unsigned int code) const
{
    modbus_reply_exception(context, data, code);
}

void modbus_server_receive(modbus_t *ctx, uint8_t *data, uint16_t size)
{
    ModbusQuery mq(ctx,data,size);
    helper_data *hd = (helper_data *)ctx->backend_data;
    hd->reply_handler(hd->user_data,mq);
}


static void modbus_helper_free(modbus_t *ctx)
{
    helper_data *hd = (helper_data *)ctx->backend_data;
    delete hd;
    delete ctx->backend;
    delete ctx;
}

int nothing(modbus_t *ctx)
{
}

modbus_t* modbus_new_helper(void *user_data, modbus_helper_receive_IT receiver, modbus_helper_reply_IT replyer)
{
    modbus_backend_t *modbus_helper_backend = modbus_create_fake_backend();
    modbus_helper_backend->send = modbus_helper_send;
    modbus_helper_backend->recv = modbus_helper_recv;
    modbus_helper_backend->connect = modbus_helper_connect;
    modbus_helper_backend->close = modbus_helper_close;
    modbus_helper_backend->select = modbus_helper_select;
    modbus_helper_backend->free = modbus_helper_free;
    modbus_helper_backend->flush = nothing;

    modbus_t *ctx = new modbus_t();
    _modbus_init_common(ctx);
    ctx->backend = modbus_helper_backend;
    helper_data *hd = new helper_data();
    hd->recv_handler = receiver;
    hd->user_data = user_data;
    hd->reply_handler = replyer;
    ctx->backend_data = hd;
    return ctx;
}

//========================================================================================

struct Interceptor
{
    std::vector<uint8_t> result;
};

static ssize_t modbus_intercept_request(modbus_t *ctx, const uint8_t *data, int dataSize)
{
    auto interceptor = static_cast<Interceptor*>(ctx->backend_data);
    for(auto it=data; it<data+dataSize-2; it++)
        interceptor->result.push_back(*it);
    return dataSize;
}

std::vector<uint8_t> modbus_create_request(std::function<void(modbus_t*)> action)
{
    modbus_backend_t *fake_backend = modbus_create_fake_backend();
    fake_backend->send = modbus_intercept_request;
    modbus_t *ctx = new modbus_t();
    _modbus_init_common(ctx);
    ctx->backend = fake_backend;
    Interceptor interceptor;
    ctx->backend_data = &interceptor;
    action(ctx);
    delete ctx->backend;
    delete ctx;
    return interceptor.result;
}

//========================================================================================

static ssize_t modbus_intercept_reply(modbus_t *ctx, uint8_t *data, int dataSize)
{
    std::vector<uint8_t> *reply = (std::vector<uint8_t> *)ctx->backend_data;
    std::copy(data, data + dataSize-2, reply->begin());
    return dataSize;
}

std::vector<uint8_t> modbus_create_reply(const uint8_t *req, int req_length, modbus_mapping_t *mb_mapping)
{
    modbus_backend_t *fake_backend = modbus_create_fake_backend();
    fake_backend->recv = modbus_intercept_reply;
    modbus_t *ctx = new modbus_t();
    _modbus_init_common(ctx);
    ctx->backend = fake_backend;
    std::vector<uint8_t> reply;
    ctx->backend_data = &reply;
    int rc = modbus_reply(ctx, req, req_length, mb_mapping);
    delete ctx->backend;
    delete ctx;
    return reply;
}


