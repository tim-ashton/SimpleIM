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
    Client *client = new Client(clientSock, std::bind(&ClientManager::onClientDisconnected, this, std::placeholders::_1));
    
    bool loginSuccessful = client->handleLogon(this);
    
    if(loginSuccessful && !client->getUserId().empty()) {
        client->run(this);
        
        // Register the client and notify others
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            m_connectedClients.emplace(client->getUserId(), std::unique_ptr<Client>(client));
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

void ClientManager::registerClient(const std::string& userId, std::unique_ptr<Client> client)
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
        it->second->sendMessage(MessageType::ChatMessage, formattedMessage);
        return true;
    }
    
    return false; // User not found
}

void ClientManager::routeChatMessage(const std::string& fromUserId, const std::string& messageContent)
{
    if (isDirectMessage(messageContent)) {
        auto [toUserId, actualMessage] = parseDirectMessage(messageContent);
        
        if (toUserId.empty() || actualMessage.empty()) {
            // Send error back to sender
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            auto it = m_connectedClients.find(fromUserId);
            if (it != m_connectedClients.end()) {
                it->second->sendMessage(MessageType::ChatMessage, "System: Invalid message format. Use @:username message");
            }
            return;
        }
        
        if (sendDirectMessage(fromUserId, toUserId, actualMessage)) {
            // Confirm to sender
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            auto it = m_connectedClients.find(fromUserId);
            if (it != m_connectedClients.end()) {
                it->second->sendMessage(MessageType::ChatMessage, "System: Message sent to " + toUserId);
            }
        } else {
            // User not found
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            auto it = m_connectedClients.find(fromUserId);
            if (it != m_connectedClients.end()) {
                it->second->sendMessage(MessageType::ChatMessage, "System: User '" + toUserId + "' not found or not online");
            }
        }
    } else {
        // Broadcast to all users (public message)
        std::string formattedMessage = fromUserId + ": " + messageContent;
        broadcastMessage(MessageType::ChatMessage, formattedMessage);
    }
}

bool ClientManager::isDirectMessage(const std::string& message)
{
    return message.size() >= 3 && message.substr(0, 2) == "@:";
}

std::pair<std::string, std::string> ClientManager::parseDirectMessage(const std::string& message)
{
    if (!isDirectMessage(message)) {
        return {"", ""};
    }
    
    // Find the space after the username
    size_t spacePos = message.find(' ', 2);
    if (spacePos == std::string::npos) {
        return {"", ""};
    }
    
    std::string toUserId = message.substr(2, spacePos - 2);
    std::string actualMessage = message.substr(spacePos + 1);
    
    return {toUserId, actualMessage};
}
