#include "ClientManager.h"

#include <functional>


ClientManager::ClientManager()
{
}

ClientManager::~ClientManager()
{
}

void ClientManager::addConnectedClient(int clientSock)
{

    Client *client = new Client(clientSock, std::bind(&ClientManager::onClientDisconnected, this, std::placeholders::_1));
    std::string userId = client->handleLogon();

    if(!userId.empty()) {
        client->run();
        m_connectedClients.emplace(userId, std::unique_ptr<Client>(client));
    }
    else {

        close(clientSock);
        delete client;
        client = nullptr;
    }

}

void ClientManager::onClientDisconnected(std::string userId)
{
    std::cout << __PRETTY_FUNCTION__ << "User id: " << userId;
    
    // TODO This is running from a different thread...
    m_connectedClients.erase(userId);
}
