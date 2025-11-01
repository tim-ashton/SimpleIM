#include "ServerClient.h"
#include "ClientManager.h"

#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <vector>
#include <unistd.h>

ServerClient::ServerClient(int socket,
               std::function<void(std::string)> disconnectCallback)
    : m_socket(socket)
    , m_userId("")
    , m_terminate(false)
    , m_clientDisconnected(disconnectCallback)
{}

ServerClient::~ServerClient()
{
    m_terminate = true;
    
    // Close socket first to interrupt any blocking recv calls
    if (m_socket > -1) {
        close(m_socket);
        m_socket = -1;
    }
    
    // Then wait for thread to finish
    if(m_thread && m_thread->joinable()) {
        m_thread->join();
    }
}

bool ServerClient::handleLogon(ClientManager* manager)
{
    std::optional<MessageHeader> header = readMessageHeader();
    if(!header.has_value()) {
        std::cerr << __PRETTY_FUNCTION__ << "Failed to read message header." << std::endl;
        return false;
    }

    if(header->type != MessageType::UserLogon) {
        std::cerr << __PRETTY_FUNCTION__ << "Incorrect login message type." << std::endl;
        sendMessage(MessageType::LoginFailure, "Invalid message type");
        return false;
    }

    std::string username = readMessageData(header->length);
    if(username.empty()) {
        std::cerr << __PRETTY_FUNCTION__ << "Empty username provided." << std::endl;
        sendMessage(MessageType::LoginFailure, "Username cannot be empty");
        return false;
    }

    // Check if username is already taken
    if(!manager->isUsernameAvailable(username)) {
        std::cout << __PRETTY_FUNCTION__ << "Username '" << username << "' already taken." << std::endl;
        sendMessage(MessageType::LoginFailure, "Username already taken");
        return false;
    }

    m_userId = username;
    
    // Send login success
    sendMessage(MessageType::LoginSuccess, "Login successful");
    
    // Send current list of connected clients
    std::string userList = manager->serializeUserList();
    sendMessage(MessageType::ConnectedClientsList, userList);
    
    std::cout << __PRETTY_FUNCTION__ << "User '" << m_userId << "' logged in successfully." << std::endl;
    return true;
}

void ServerClient::run(ClientManager* manager)
{
    if (!m_thread)
    {
        std::cout << __PRETTY_FUNCTION__ << "Client starting..." << std::endl;

        m_thread.reset(new std::thread([this, manager]()->void {
            
            while(!m_terminate && m_socket > -1) {

                std::optional<MessageHeader> header = readMessageHeader();

                if(header.has_value() && !m_terminate) {

                    std::string messageData = readMessageData(header->length);

                    if (!m_terminate) {
                        switch(header->type)
                        {
                            case MessageType::UserLogon:
                                std::cout << "User logon during active session." << std::endl;
                                m_userId = messageData;
                            break;
                            case MessageType::UserLogoff:
                            break;
                            case MessageType::ChatMessageBroadcast:
                                std::cout << "Received chat message from " << m_userId << ": " << messageData << std::endl;
                                if (manager) {
                                    manager->broadcastChatMessage(m_userId, messageData);
                                }
                            break;
                            case MessageType::ChatMessageDM:
                                std::cout << "Received direct message from " << m_userId << ": " << messageData << std::endl;
                                if (manager) {
                                    manager->handleDirectMessage(m_userId, messageData);
                                }
                            break;
                        }
                    }
                }
                else {
                    // If we can't read a header, the client has likely disconnected
                    break;
                }
            }
            
            std::cout << __PRETTY_FUNCTION__ << "Client thread ending for user: " << m_userId << std::endl;
        }));
    }
    else
    {
        std::cerr << __PRETTY_FUNCTION__  << "Client thread already running. Run called twice!";
    }
}

std::optional<MessageHeader> ServerClient::readMessageHeader()
{
    if (m_socket > -1 && !m_terminate)
    {
        char headerBuffer[5];
        int bytesReceived = recv(m_socket, headerBuffer, sizeof(headerBuffer), 0);
        if(bytesReceived == 0) {
            // Client disconnected normally
            std::cout << __PRETTY_FUNCTION__ << "Client disconnected." << std::endl;
            handleSocketError();
        }
        else if (bytesReceived == -1)
        {
            if (!m_terminate) {
                std::cerr << __PRETTY_FUNCTION__ << "Error: Could not receive data from client" << std::endl;
            }
            handleSocketError();
            return std::nullopt;
        }
        else if (bytesReceived < sizeof(headerBuffer))
        {
            std::cerr << __PRETTY_FUNCTION__ << "Error: Incomplete header received" << std::endl;
            handleSocketError();
            return std::nullopt;
        }

        MessageType type = static_cast<MessageType>(headerBuffer[0]);
        uint32_t payloadLength = 0;
        std::memcpy(&payloadLength, &headerBuffer[1], sizeof(uint32_t));

        return MessageHeader(type, payloadLength);
    }
    else
    {
        std::cerr << __PRETTY_FUNCTION__ << "Invalid socket.";
        return std::nullopt;
    }
}

std::string ServerClient::readMessageData(uint32_t dataLen)
{
    if(dataLen < 1){
        std::cout << __PRETTY_FUNCTION__ << "No data.";
        return std::string();
    }

    if (m_socket > -1) {

        std::vector<char> payloadBuffer(dataLen);
        size_t bytesReceived = recv(m_socket, payloadBuffer.data(), dataLen, 0);

        if (bytesReceived == -1) {
            if (!m_terminate) {
                std::cerr << __PRETTY_FUNCTION__ << "Error: Could not receive payload from client" << std::endl;
            }
            handleSocketError();
            return std::string();
        } 
        else if (bytesReceived < dataLen) {
            std::cerr << __PRETTY_FUNCTION__ << "Error: Incomplete payload received" << std::endl;
            handleSocketError();
            return std::string();
        }

        std::string payload(payloadBuffer.begin(), payloadBuffer.end());
        return payload;
    }
    else {
        std::cerr << __PRETTY_FUNCTION__ << "Invalid socket.";
        return std::string();
    }
}

void ServerClient::handleSocketError()
{
    m_terminate = true;

    if (m_socket > -1) {
        close(m_socket);
        m_socket = -1;
    }

    // Only call disconnect callback once
    if(m_clientDisconnected) {
        auto callback = m_clientDisconnected;
        m_clientDisconnected = nullptr;  // Prevent multiple calls
        callback(m_userId);
    }
}

bool ServerClient::sendMessage(MessageType type, const std::string& data)
{
    if (m_socket <= -1) {
        std::cerr << __PRETTY_FUNCTION__ << "Invalid socket." << std::endl;
        return false;
    }

    // Create header
    MessageHeader header(type, static_cast<uint32_t>(data.length()));
    
    // Send header
    char headerBuffer[5];
    headerBuffer[0] = static_cast<uint8_t>(type);
    std::memcpy(&headerBuffer[1], &header.length, sizeof(uint32_t));
    
    int bytesSent = send(m_socket, headerBuffer, sizeof(headerBuffer), 0);
    if (bytesSent != sizeof(headerBuffer)) {
        std::cerr << __PRETTY_FUNCTION__ << "Failed to send header" << std::endl;
        handleSocketError();
        return false;
    }
    
    // Send data if any
    if (!data.empty()) {
        bytesSent = send(m_socket, data.c_str(), data.length(), 0);
        if (bytesSent != static_cast<int>(data.length())) {
            std::cerr << __PRETTY_FUNCTION__ << "Failed to send data" << std::endl;
            handleSocketError();
            return false;
        }
    }
    
    return true;
}
