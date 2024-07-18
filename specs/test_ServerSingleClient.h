#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <thread>
#include <mutex>

#include "modbus.h"
#include "server.h"
#include "log.h"

#include "ServerBase.h"

using namespace testing;

class ServerTestSingleClient : public ServerTestBase<3502>
{
public:
    void SetUp();
    modbus_t *context;
    void TearDown();
};

void ServerTestSingleClient::SetUp()
{
    context = modbus_new_tcp(ServerAddress, ServerPort);
}

void ServerTestSingleClient::TearDown()
{
    modbus_free(context);
}

TEST_F(ServerTestSingleClient, HandleSingleRequest)
{
    DefineRequestHandler([](ModbusServer *s, uint8_t *query, int sz){
        modbus_mapping_t *mm = modbus_mapping_new_start_address(0, 0, 0, 0, 1234, 7, 0, 0);
        uint16_t response[] = { 0xffff, 0xeeee, 0xdddd, 0xcccc, 0xbbbb, 0xaaaa };
        std::copy(std::begin(response), std::end(response), mm->tab_registers);
        mm->tab_registers[6] = query[6];
        modbus_reply(s->context, query, sz, mm);
        modbus_mapping_free(mm);
    });
    ASSERT_THAT(modbus_connect(context), Eq(0));
    modbus_set_slave(context, 0x99);
    uint16_t modbus_response[7];
    int rc = modbus_read_registers(context, 1234, 7, modbus_response);
    modbus_close(context);
    ASSERT_THAT(rc, Eq(7));
    ASSERT_THAT(modbus_response[0], Eq(0xffff));
    ASSERT_THAT(modbus_response[1], Eq(0xeeee));
    ASSERT_THAT(modbus_response[2], Eq(0xdddd));
    ASSERT_THAT(modbus_response[3], Eq(0xcccc));
    ASSERT_THAT(modbus_response[4], Eq(0xbbbb));
    ASSERT_THAT(modbus_response[5], Eq(0xaaaa));
    ASSERT_THAT(modbus_response[6], Eq(0x99));
}

TEST_F(ServerTestSingleClient, HandleMultipleRequests)
{
    DefineRequestHandler([](ModbusServer *s, uint8_t *query, int sz){
        static uint16_t counter = 15;
        modbus_mapping_t *mm = modbus_mapping_new_start_address(0, 0, 0, 0, 1234, 2, 0, 0);
        uint8_t slave = query[6];
        mm->tab_registers[0] = slave;
        mm->tab_registers[1] = counter++;
        modbus_reply(s->context, query, sz, mm);
        modbus_mapping_free(mm);
    });
    ASSERT_THAT(modbus_connect(context), Eq(0));
    {
        modbus_set_slave(context, 0x11);
        uint16_t modbus_response[2];
        int rc = modbus_read_registers(context, 1234, 2, modbus_response);
        ASSERT_THAT(rc, Eq(2));
        ASSERT_THAT(modbus_response[0], Eq(0x11));
        ASSERT_THAT(modbus_response[1], Eq(15));
    }
    {
        modbus_set_slave(context, 0x23);
        uint16_t modbus_response[2];
        int rc = modbus_read_registers(context, 1234, 2, modbus_response);
        ASSERT_THAT(rc, Eq(2));
        ASSERT_THAT(modbus_response[0], Eq(0x23));
        ASSERT_THAT(modbus_response[1], Eq(16));
    }
    {
        modbus_set_slave(context, 0x4e);
        uint16_t modbus_response[2];
        int rc = modbus_read_registers(context, 1234, 2, modbus_response);
        ASSERT_THAT(rc, Eq(2));
        ASSERT_THAT(modbus_response[0], Eq(0x4e));
        ASSERT_THAT(modbus_response[1], Eq(17));
    }
    modbus_close(context);
}


