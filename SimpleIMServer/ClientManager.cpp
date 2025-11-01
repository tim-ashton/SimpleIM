#include "ClientManager.h"

#include <functional>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <sstream>


ClientManager::ClientManager()
{
}

ClientManager::~ClientManager()
{
}

void ClientManager::addConnectedClient(int clientSock)
{
    ServerClient *client = new ServerClient(clientSock, std::bind(&ClientManager::onClientDisconnected, this, std::placeholders::_1));
    
    bool loginSuccessful = client->handleLogon(this);
    
    if(loginSuccessful && !client->getUserId().empty()) {
        client->run(this);
        
        // Register the client and notify others
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            m_connectedClients.emplace(client->getUserId(), std::unique_ptr<ServerClient>(client));
        }
        
        // Broadcast to other clients that a new user connected
        broadcastToOthers(client->getUserId(), MessageType::ClientConnected, client->getUserId());
        
        std::cout << "Client '" << client->getUserId() << "' connected successfully." << std::endl;
    }
    else {
        std::cout << "Client connection failed during login." << std::endl;
        close(clientSock);
        delete client;
        client = nullptr;
    }
}

bool ClientManager::isUsernameAvailable(const std::string& username)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    return m_connectedClients.find(username) == m_connectedClients.end();
}

void ClientManager::registerClient(const std::string& userId, std::unique_ptr<ServerClient> client)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    m_connectedClients.emplace(userId, std::move(client));
}

void ClientManager::broadcastMessage(MessageType type, const std::string& data)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for(auto& clientPair : m_connectedClients) {
        clientPair.second->sendMessage(type, data);
    }
}

void ClientManager::broadcastToOthers(const std::string& excludeUserId, MessageType type, const std::string& data)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for(auto& clientPair : m_connectedClients) {
        if(clientPair.first != excludeUserId) {
            clientPair.second->sendMessage(type, data);
        }
    }
}

std::vector<std::string> ClientManager::getConnectedUsernames()
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    std::vector<std::string> usernames;
    usernames.reserve(m_connectedClients.size());
    
    for(const auto& clientPair : m_connectedClients) {
        usernames.push_back(clientPair.first);
    }
    
    return usernames;
}

std::string ClientManager::serializeUserList()
{
    std::vector<std::string> usernames = getConnectedUsernames();
    std::ostringstream oss;
    
    for(size_t i = 0; i < usernames.size(); ++i) {
        if(i > 0) oss << ",";
        oss << usernames[i];
    }
    
    return oss.str();
}

void ClientManager::onClientDisconnected(std::string userId)
{
    std::cout << __PRETTY_FUNCTION__ << "User id: " << userId << std::endl;
    
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        m_connectedClients.erase(userId);
    }
    
    // Broadcast to all remaining clients that someone disconnected
    broadcastMessage(MessageType::ClientDisconnected, userId);
    
    std::cout << "Client '" << userId << "' disconnected and removed." << std::endl;
}

bool ClientManager::sendDirectMessage(const std::string& fromUserId, const std::string& toUserId, const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    
    auto it = m_connectedClients.find(toUserId);
    if (it != m_connectedClients.end()) {
        // Format: "fromUser: message"
        std::string formattedMessage = fromUserId + ": " + message;
        it->second->sendMessage(MessageType::ChatMessageBroadcast, formattedMessage);
        return true;
    }
    
    return false; // User not found
}

void ClientManager::broadcastChatMessage(const std::string& fromUserId, const std::string& message)
{
    // Broadcast to all users (public message)
    std::string formattedMessage = fromUserId + ": " + message;
    broadcastMessage(MessageType::ChatMessageBroadcast, formattedMessage);
}

void ClientManager::handleDirectMessage(const std::string& fromUserId, const std::string& messageData)
{
    // Parse the direct message format: "targetUser:message"
    size_t colonPos = messageData.find(':');
    if (colonPos == std::string::npos || colonPos == 0 || colonPos == messageData.length() - 1) {
        // Send error back to sender - invalid format
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_connectedClients.find(fromUserId);
        if (it != m_connectedClients.end()) {
            it->second->sendMessage(MessageType::ChatMessageBroadcast, "System: Invalid direct message format. Expected 'username:message'");
        }
        return;
    }
    
    std::string toUserId = messageData.substr(0, colonPos);
    std::string actualMessage = messageData.substr(colonPos + 1);
    
    if (toUserId.empty() || actualMessage.empty()) {
        // Send error back to sender
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_connectedClients.find(fromUserId);
        if (it != m_connectedClients.end()) {
            it->second->sendMessage(MessageType::ChatMessageBroadcast, "System: Username and message cannot be empty");
        }
        return;
    }
    
    if (sendDirectMessage(fromUserId, toUserId, actualMessage)) {
        // Confirm to sender
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_connectedClients.find(fromUserId);
        if (it != m_connectedClients.end()) {
            it->second->sendMessage(MessageType::ChatMessageBroadcast, "System: Message sent to " + toUserId);
        }
    } else {
        // User not found
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_connectedClients.find(fromUserId);
        if (it != m_connectedClients.end()) {
            it->second->sendMessage(MessageType::ChatMessageBroadcast, "System: User '" + toUserId + "' not found or not online");
        }
    }
}
