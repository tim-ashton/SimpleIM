#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <optional>

#include "SimpleIMClient.h"


SimpleIMClient::SimpleIMClient()
{}

SimpleIMClient::~SimpleIMClient()
{
    disconnectFromServer();
}

bool SimpleIMClient::connected()
{
    return m_connected;
}

void SimpleIMClient::logon(const std::string &username)
{
    if(!connectToServer()) {
        std::cout << __FUNCTION__ << "Unable to connect to SimpleIM Server. Exiting." << std::endl;
        return;
    }
    
    // Start receiving messages in background
    startReceiveThread();
    
    // Send login message
    sendMessage(Message(MessageType::UserLogon, username));
}

void SimpleIMClient::sendChatMessage(const std::string &message)
{
    sendMessage(Message(MessageType::ChatMessageBroadcast, message));
}

void SimpleIMClient::sendDirectMessage(const std::string &targetUsername, const std::string &message)
{
    // Format: "targetUser:message"
    std::string dmData = targetUsername + ":" + message;
    sendMessage(Message(MessageType::ChatMessageDM, dmData));
}

bool SimpleIMClient::connectToServer()
{
    m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_clientSocket == -1) {
        std::cerr << __FUNCTION__ << "SimpleIMClient::connectToServer() Error: Could not create socket\n";
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8989);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(m_clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << __FUNCTION__ << "SimpleIMClient::connectToServer() Error: Could not connect to server\n";
        close(m_clientSocket);
        return false;
    }

    std::cout << __FUNCTION__ << "connection successful" << std::endl;
    m_connected = true;
    return m_connected;
}

void SimpleIMClient::disconnectFromServer()
{
    if(m_connected) {
        m_terminate = true;
        
        // Close socket first to interrupt any blocking recv calls
        if (m_clientSocket > -1) {
            close(m_clientSocket);
            m_clientSocket = -1;
        }
        
        // Then join the receive thread if it exists
        if(m_receiveThread && m_receiveThread->joinable()) {
            m_receiveThread->join();
            m_receiveThread.reset();
        }
        
        m_connected = false;
        std::cout << "Disconnected from server." << std::endl;
    }
}

void SimpleIMClient::sendMessage(const Message& message)
{
    if(!m_connected) {
        std::cerr << __FUNCTION__ << "Warning: not connected. Cannot send." << std::endl;
        return;
    }

    std::vector<uint8_t> data = message.to_bytes();
    if (send(m_clientSocket, data.data(), data.size(), 0) == -1) {
        std::cerr << __FUNCTION__ << "Could not send to server." << std::endl;

        m_connected = false;
        close(m_clientSocket);
        return;
    }
}

void SimpleIMClient::startReceiveThread()
{
    if (!m_receiveThread) {
        m_receiveThread.reset(new std::thread([this]() {
            receiveMessages();
        }));
    }
}

void SimpleIMClient::receiveMessages()
{
    while (!m_terminate && m_connected && m_clientSocket > -1) {
        std::optional<MessageHeader> header = readMessageHeader();
        if (header.has_value() && !m_terminate) {
            std::string data = readMessageData(header->length);
            if (!m_terminate) {
                handleReceivedMessage(header->type, data);
            }
        }
        else {
            // If we can't read a header, connection is likely lost
            break;
        }
    }
    
    std::cout << "Receive thread ending." << std::endl;
}

std::optional<MessageHeader> SimpleIMClient::readMessageHeader()
{
    if (m_clientSocket <= -1) {
        return std::nullopt;
    }

    char headerBuffer[5];
    int bytesReceived = recv(m_clientSocket, headerBuffer, sizeof(headerBuffer), 0);
    
    if (bytesReceived == 0) {
        std::cout << "Server disconnected." << std::endl;
        m_connected = false;
        return std::nullopt;
    }
    else if (bytesReceived == -1) {
        std::cerr << __FUNCTION__ << "Error: Could not receive data from server" << std::endl;
        m_connected = false;
        return std::nullopt;
    }
    else if (bytesReceived < sizeof(headerBuffer)) {
        std::cerr << __FUNCTION__ << "Error: Incomplete header received" << std::endl;
        m_connected = false;
        return std::nullopt;
    }

    MessageType type = static_cast<MessageType>(headerBuffer[0]);
    uint32_t payloadLength = 0;
    std::memcpy(&payloadLength, &headerBuffer[1], sizeof(uint32_t));

    return MessageHeader(type, payloadLength);
}

std::string SimpleIMClient::readMessageData(uint32_t dataLen)
{
    if (dataLen < 1) {
        return std::string();
    }

    if (m_clientSocket <= -1) {
        return std::string();
    }

    std::vector<char> payloadBuffer(dataLen);
    size_t bytesReceived = recv(m_clientSocket, payloadBuffer.data(), dataLen, 0);

    if (bytesReceived == -1) {
        std::cerr << __FUNCTION__ << "Error: Could not receive payload from server" << std::endl;
        m_connected = false;
        return std::string();
    }
    else if (bytesReceived < dataLen) {
        std::cerr << __FUNCTION__ << "Error: Incomplete payload received" << std::endl;
        m_connected = false;
        return std::string();
    }

    return std::string(payloadBuffer.begin(), payloadBuffer.end());
}

void SimpleIMClient::handleReceivedMessage(MessageType type, const std::string& data)
{
    switch (type) {
        case MessageType::LoginSuccess:
            handleLoginSuccess(data);
            break;
        case MessageType::LoginFailure:
            handleLoginFailure(data);
            break;
        case MessageType::ConnectedClientsList:
            handleConnectedClientsList(data);
            break;
        case MessageType::ClientConnected:
            handleClientConnected(data);
            break;
        case MessageType::ClientDisconnected:
            handleClientDisconnected(data);
            break;
        case MessageType::ChatMessageBroadcast:
            std::cout << "Chat: " << data << std::endl;
            break;
        case MessageType::ChatMessageDM:
            std::cout << "Direct Message: " << data << std::endl;
            break;
        default:
            std::cout << "Received unknown message type." << std::endl;
            break;
    }
}

void SimpleIMClient::handleLoginSuccess(const std::string& data)
{
    std::cout << "✓ Login successful! " << data << std::endl;
}

void SimpleIMClient::handleLoginFailure(const std::string& data)
{
    std::cout << "✗ Login failed: " << data << std::endl;
    disconnectFromServer();
}

void SimpleIMClient::handleConnectedClientsList(const std::string& data)
{
    if (data.empty()) {
        std::cout << "Connected users: (none)" << std::endl;
        return;
    }
    
    std::cout << "Connected users: " << data << std::endl;
}

void SimpleIMClient::handleClientConnected(const std::string& data)
{
    std::cout << "User '" << data << "' joined the chat." << std::endl;
}

void SimpleIMClient::handleClientDisconnected(const std::string& data)
{
    std::cout << "User '" << data << "' left the chat." << std::endl;
}
