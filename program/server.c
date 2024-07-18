#include "server.h"
#include "log.h"

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define LOCAL_HOST "127.0.0.1"

ModbusServer *ModbusServer_create(int port)
{
    ModbusServer *server = malloc(sizeof(ModbusServer));
    server->context = modbus_new_tcp(NULL, port);
    modbus_set_debug(server->context,IsLogDebug());
    server->serverSocket = modbus_tcp_listen(server->context, 1);
    FD_ZERO(&server->sockets);
    FD_SET(server->serverSocket, &server->sockets);
    server->maxSocket = server->serverSocket;
    server->remoteBlocked = false;
    return server;
}

static bool FD_ISREMOTE(int socket)
{
    struct sockaddr_in address;
    socklen_t address_len;
    char ipAddr[16] = {'\0'};
    getsockname(socket,(struct sockaddr*)&address,&address_len);
    inet_ntop(AF_INET,&address.sin_addr,ipAddr,address_len);
    return (!strcmp(ipAddr,LOCAL_HOST)) ? false : true;
}

void ModbusServer_setHandler(ModbusServer *server, ModbusServerRequestHandler requestHandler, void *userData)
{
    server->handler = requestHandler;
    server->userData = userData;
}

void ModbusServer_enableRemoteConnections(ModbusServer *server)
{
    LOG_INFO("Allowing remote clients back\n");
    server->remoteBlocked = false;

}

static void ModbusServer_removeClient(ModbusServer *server, int socket)
{
    LOG_DEBUG("Closing existing connection on socket %d\n",socket);
    close(socket);
    FD_CLR(socket, &server->sockets);
}

void ModbusServer_disableRemoteConnections(ModbusServer *server)
{
    for (int fd = 0; fd < server->maxSocket; fd++)
    {
        if (FD_ISSET(fd,&(server->sockets)) && FD_ISREMOTE(fd) && (fd != server->serverSocket))
            ModbusServer_removeClient(server,fd);
    }
    LOG_INFO("Every remote clients have been disconnected. Blocking new remote connections...\n");
    server->remoteBlocked = true;
}

static void ModbusServer_addClient(ModbusServer *server)
{
    int clientSocket = modbus_tcp_accept(server->context,&server->serverSocket);
    if (FD_ISREMOTE(clientSocket) && server->remoteBlocked)
    {
        LOG_WARNING("New remote connection, closing socket\n");
        close(clientSocket);
        return;
    }
    LOG_DEBUG("New connection on socket %d\n",clientSocket);
    FD_SET(clientSocket, &server->sockets);
    if (clientSocket > server->maxSocket)
        server->maxSocket = clientSocket;
}

static void ModbusServer_handleClient(ModbusServer *server, int socket)
{
    if(socket == server->serverSocket)
    {
        ModbusServer_addClient(server);
        return;
    }
    modbus_set_socket(server->context, socket);
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    int rc = modbus_receive(server->context, query);
    if(rc == -1)
    {
        ModbusServer_removeClient(server, socket);
        return;
    }
    LOG_DEBUG("Request on existing socket %d\n",socket);
    server->handler(server,query,rc,server->userData);
}

void ModbusServer_run(ModbusServer *server)
{
    LOG_DEBUG("ModbusServer_run\n");
    fd_set currentSockets = server->sockets;
    select(server->maxSocket+1, &currentSockets, NULL, NULL, NULL);
    for(int socket = 0; socket <= server->maxSocket; socket++)
    {
        if(FD_ISSET(socket, &currentSockets))
            ModbusServer_handleClient(server,socket);
    }
}

void ModbusServer_delete(ModbusServer *server)
{
    if(server->serverSocket != -1)
        close(server->serverSocket);
    modbus_free(server->context);
    free(server);
}
