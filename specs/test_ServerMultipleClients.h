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

class ServerTestMultipleClients : public ServerTestBase<3503>
{
};

TEST_F(ServerTestMultipleClients, HandleSingleRequest)
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
    std::vector<modbus_t *> clients;
    for(auto clientID=0; clientID<10; clientID++)
    {
        auto context = modbus_new_tcp(ServerAddress, ServerPort);
        ASSERT_THAT(modbus_connect(context), Eq(0));
        modbus_set_slave(context, 0xa0 + clientID);
        clients.push_back(context);
    }
    auto i=0;
    for(auto client=clients.begin(); client!=clients.end(); client++)
    {
        uint16_t modbus_response[2];
        int rc = modbus_read_registers(*client, 1234, 2, modbus_response);
        modbus_close(*client);
        ASSERT_THAT(rc, Eq(2));
        ASSERT_THAT(modbus_response[0], Eq(0xa0 + i));
        ASSERT_THAT(modbus_response[1], Eq(15 + i));
        i++;
    }
}



