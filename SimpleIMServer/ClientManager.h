#pragma once

#include "Client.h"

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
    void registerClient(const std::string& userId, std::unique_ptr<Client> client);
    
    void broadcastMessage(MessageType type, const std::string& data = "");
    void broadcastToOthers(const std::string& excludeUserId, MessageType type, const std::string& data = "");
    
    // Direct messaging functionality
    bool sendDirectMessage(const std::string& fromUserId, const std::string& toUserId, const std::string& message);
    void routeChatMessage(const std::string& fromUserId, const std::string& messageContent);
    
    std::vector<std::string> getConnectedUsernames();
    std::string serializeUserList();

    void onClientDisconnected(std::string userId);

private:
    std::unordered_map<std::string, std::unique_ptr<Client>> m_connectedClients;
    std::mutex m_clientsMutex;
    
    // Helper methods for message parsing
    bool isDirectMessage(const std::string& message);
    std::pair<std::string, std::string> parseDirectMessage(const std::string& message);
};