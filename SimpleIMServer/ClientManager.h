#pragma once

#include "Client.h"

#include <unordered_map>

class ClientManager
{
public:
    ClientManager();
    ~ClientManager();

    void addConnectedClient(int clientSock);

    void onClientDisconnected(std::string userId);

private:
    std::unordered_map<std::string, std::unique_ptr<Client>> m_connectedClients;
};