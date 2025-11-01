#pragma once

#include "ServerClient.h"

#include <unordered_map>
#include <mutex>
#include <vector>

class ClientManager
{
public:
    ClientManager();
    ~ClientManager();

    void addConnectedClient(int clientSock);
    bool isUsernameAvailable(const std::string& username);
    void registerClient(const std::string& userId, std::unique_ptr<ServerClient> client);
    
    void broadcastMessage(MessageType type, const std::string& data = "");
    void broadcastToOthers(const std::string& excludeUserId, MessageType type, const std::string& data = "");
    
    // Chat messaging functionality
    void broadcastChatMessage(const std::string& fromUserId, const std::string& message);
    void handleDirectMessage(const std::string& fromUserId, const std::string& messageData);
    bool sendDirectMessage(const std::string& fromUserId, const std::string& toUserId, const std::string& message);
    
    std::vector<std::string> getConnectedUsernames();
    std::string serializeUserList();

    void onClientDisconnected(std::string userId);

private:
    std::unordered_map<std::string, std::unique_ptr<ServerClient>> m_connectedClients;
    std::mutex m_clientsMutex;
};