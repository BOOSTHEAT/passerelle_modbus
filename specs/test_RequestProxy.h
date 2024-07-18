#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "modbus.h"
#include "modbus_helper.h"
#include "log.h"
#include "proxy.h"

using namespace testing;

class RequestProxyTest : public ::testing::Test
{
public:
    void SetUp();
    void TearDown();

    modbus_t *proxyListenAndReplyToMaster;
    bool expectationsAreVerified;
    static void sendReplyToMaster(void*,const uint8_t *,int);
    std::function<void(const uint8_t *response, int responseSize)> expectedRepliesToMaster;
    std::vector<uint16_t> ExtractRegistersFromResponse(const uint8_t *response, int responseSize);
    unsigned int GetExceptionCode(const uint8_t *response);

    modbus_t *proxySendToSlave;
    static void sendRequestToSlave(void*,const uint8_t *,int);
    std::function<void(const ModbusQuery &mq)> SlaveReply;

    modbus_t *slaveListenAndReplyToProxy;
    static void sendReplyToProxy(void*,const uint8_t *,int);

    ModbusProxy *sut;
};

#define ExpectReplyToMaster(e) \
    ASSERT_TRUE(expectationsAreVerified); \
    expectationsAreVerified = false; \
    expectedRepliesToMaster = e;

void RequestProxyTest::SetUp()
{
    proxyListenAndReplyToMaster = modbus_new_helper(this,sendReplyToMaster,nullptr);
    proxySendToSlave = modbus_new_helper(this,sendRequestToSlave,nullptr);
    sut = ModbusProxy_create(proxySendToSlave,0,2);
    slaveListenAndReplyToProxy = modbus_new_helper(this,sendReplyToProxy,nullptr);
    expectedRepliesToMaster = [](const uint8_t *, int){};
    SlaveReply = [](const ModbusQuery &){};
    expectationsAreVerified = true;
}

void RequestProxyTest::TearDown()
{
    ASSERT_TRUE(expectationsAreVerified);
    ModbusProxy_delete(sut);
    modbus_free(proxyListenAndReplyToMaster);
    modbus_free(proxySendToSlave);
}

void RequestProxyTest::sendReplyToMaster(void *s,const uint8_t *req, int req_length)
{
    auto self = static_cast<RequestProxyTest*>(s);
    self->expectedRepliesToMaster(req,req_length);
    self->expectationsAreVerified = true;
}

std::vector<uint16_t> RequestProxyTest::ExtractRegistersFromResponse(const uint8_t *response, int responseSize)
{
    int registerCount = (responseSize - 5)/2;
    std::vector<uint16_t> registers;
    for (auto i = 0; i < registerCount; i++)
        registers.push_back((response[3 + (i << 1)] << 8) | response[4 + (i << 1)]);
    return registers;
}

unsigned int RequestProxyTest::GetExceptionCode(const uint8_t *response)
{
    return response[2];
}

void RequestProxyTest::sendRequestToSlave(void *s,const uint8_t *req, int req_length)
{
    auto self = static_cast<RequestProxyTest*>(s);
    ModbusQuery mq(self->slaveListenAndReplyToProxy,(uint8_t *)req,req_length);
    self->SlaveReply(mq);
}

void RequestProxyTest::sendReplyToProxy(void *s,const uint8_t *req, int req_length)
{
    auto self = static_cast<RequestProxyTest*>(s);
    modbus_transmit(self->proxySendToSlave, (uint8_t *)req, req_length);
}


TEST_F(RequestProxyTest, SlaveDoesNotReply)
{
    auto request = modbus_create_request([](modbus_t *ctx){
        modbus_set_slave(ctx, 0x99);
        uint16_t modbus_response[2];
        modbus_read_registers(ctx, 1234, 2, modbus_response);
    });
    ExpectReplyToMaster([this](const uint8_t *response, int responseSize){
        ASSERT_THAT(GetExceptionCode(response), Eq(MODBUS_EXCEPTION_GATEWAY_TARGET));
    });
    modbus_set_slave(proxyListenAndReplyToMaster, 0x99);
    ModbusProxy_HandleRequest(sut, proxyListenAndReplyToMaster, 0, request.data(), request.size());
}

TEST_F(RequestProxyTest, SlaveReturnsException)
{
    SlaveReply = [](const ModbusQuery &mq){
        mq.ReplyException(MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE);
    };
    auto request = modbus_create_request([](modbus_t *ctx){
        modbus_set_slave(ctx, 0x99);
        uint16_t modbus_response[2];
        modbus_read_registers(ctx, 1234, 2, modbus_response);
    });
    ExpectReplyToMaster([this](const uint8_t *response, int responseSize){
        ASSERT_THAT(GetExceptionCode(response), Eq(MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE));
    });
    modbus_set_slave(proxyListenAndReplyToMaster, 0x99);
    ModbusProxy_HandleRequest(sut, proxyListenAndReplyToMaster, 0, request.data(), request.size());
}

TEST_F(RequestProxyTest, ReadHoldingRegisters)
{
    SlaveReply = [](const ModbusQuery &mq){
        static uint16_t counter = 15;
        std::vector<uint16_t> response;
        response.push_back(mq.Slave());
        response.push_back(counter++);
        mq.ReplyRegisters(response);
    };

    {
        auto request = modbus_create_request([](modbus_t *ctx){
            modbus_set_slave(ctx, 0x99);
            uint16_t modbus_response[2];
            modbus_read_registers(ctx, 1234, 2, modbus_response);
        });
        ExpectReplyToMaster([this](const uint8_t *response, int responseSize){
            auto registers = ExtractRegistersFromResponse(response,responseSize);
            ASSERT_THAT(registers.size(), Eq(2));
            ASSERT_THAT(registers[0], Eq(0x99));
            ASSERT_THAT(registers[1], Eq(15));
        });
        modbus_set_slave(proxyListenAndReplyToMaster, 0x99);
        ModbusProxy_HandleRequest(sut, proxyListenAndReplyToMaster, 0, request.data(), request.size());
    }

    {
        auto request = modbus_create_request([](modbus_t *ctx){
            modbus_set_slave(ctx, 0x10);
            uint16_t modbus_response[2];
            modbus_read_registers(ctx, 1234, 2, modbus_response);
        });
        ExpectReplyToMaster([this](const uint8_t *response, int responseSize){
            auto registers = ExtractRegistersFromResponse(response,responseSize);
            ASSERT_THAT(registers.size(), Eq(2));
            ASSERT_THAT(registers[0], Eq(0x10));
            ASSERT_THAT(registers[1], Eq(16));
        });
        modbus_set_slave(proxyListenAndReplyToMaster, 0x10);
        ModbusProxy_HandleRequest(sut, proxyListenAndReplyToMaster, 0, request.data(), request.size());
    }
}
