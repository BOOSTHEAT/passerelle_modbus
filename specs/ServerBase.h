#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <thread>
#include <mutex>

#include "modbus.h"
#include "server.h"
#include "log.h"

template <int P>
class ServerTestBase : public ::testing::Test
{
public:
    static void SetUpTestSuite();
    typedef std::function<void(ModbusServer*,uint8_t*,int)> UserCallback;
    static void DefineRequestHandler(UserCallback);
    static void TearDownTestSuite();
    const int ServerPort = P;
    static const char *ServerAddress;
    static ModbusServer *sut;
private:
    static std::thread *sutThread;
    static bool running;
    static std::mutex ready;
    static UserCallback userCallback;
    static void MyModbusServerRequestCallback(ModbusServer *, uint8_t *, int, void *);
};

template <int P> std::thread *ServerTestBase<P>::sutThread;
template <int P> bool ServerTestBase<P>::running = true;
template <int P> std::mutex ServerTestBase<P>::ready;
template <int P> ModbusServer *ServerTestBase<P>::sut;
template <int P> typename ServerTestBase<P>::UserCallback ServerTestBase<P>::userCallback;
template <int P> const char *ServerTestBase<P>::ServerAddress = "127.0.0.1";

template <int P>
void ServerTestBase<P>::MyModbusServerRequestCallback(ModbusServer *server, uint8_t *query, int sz, void *d)
{
    auto callback = static_cast<UserCallback*>(d);
    (*callback)(server,query,sz);
}

template <int P>
void ServerTestBase<P>::SetUpTestSuite()
{
    ready.lock();
     sutThread = new std::thread([](){
        sut = ModbusServer_create(P);
        ready.lock();
        ModbusServer_setHandler(sut,MyModbusServerRequestCallback,&userCallback);
        while(running)
        {
            ModbusServer_run(sut);
        }
        ModbusServer_delete(sut);
    });
}

template <int P>
void ServerTestBase<P>::DefineRequestHandler(UserCallback uc)
{
    userCallback = uc;
    ready.unlock();
}

template <int P>
void ServerTestBase<P>::TearDownTestSuite()
{
    running = false;
    auto context = modbus_new_tcp(ServerAddress, P);
    modbus_connect(context);
    sutThread->join();
    delete sutThread;
}

