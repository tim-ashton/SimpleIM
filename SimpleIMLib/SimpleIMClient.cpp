#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <sys/select.h>
#include <errno.h>

#include "SimpleIMClient.h"


SimpleIMClient::SimpleIMClient()
    : m_clientUsername("")
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
    
    m_clientUsername = username;
    startNetworkThread();
    
    // Send login message
    queueMessage(Message(MessageType::UserLogon, username));
}

void SimpleIMClient::sendChatMessage(const std::string &message)
{
    queueMessage(Message(MessageType::ChatMessageBroadcast, message));
}

void SimpleIMClient::sendDirectMessage(const std::string &targetUsername, const std::string &message)
{
    // Format: "targetUser:message"
    std::string dmData = targetUsername + ":" + message;
    queueMessage(Message(MessageType::ChatMessageDM, dmData));
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
        
        // Wake up the network thread
        m_outgoingMessages.wakeUp();
        
        // Close socket first to interrupt any blocking recv calls
        if (m_clientSocket > -1) {
            close(m_clientSocket);
            m_clientSocket = -1;
        }
        
        // Then join the network thread if it exists
        if(m_networkThread && m_networkThread->joinable()) {
            m_networkThread->join();
            m_networkThread.reset();
        }
        
        m_connected = false;
        std::cout << "Disconnected from server." << std::endl;
    }
}

void SimpleIMClient::queueMessage(const Message& message)
{
    if(!m_connected) {
        std::cerr << __FUNCTION__ << "Warning: not connected. Cannot queue message." << std::endl;
        return;
    }
    
    m_outgoingMessages.push(message);
}

void SimpleIMClient::sendQueuedMessage(const Message& message)
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

void SimpleIMClient::startNetworkThread()
{
    if (!m_networkThread) {
        m_networkThread.reset(new std::thread([this]() {
            networkLoop();
        }));
    }
}

void SimpleIMClient::networkLoop()
{
    std::cout << "Network thread started" << std::endl;
    
    while (!m_terminate && m_connected && m_clientSocket > -1) {
        fd_set readfds, writefds;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(m_clientSocket, &readfds);
        
        // Only set write fd if we have messages to send
        bool hasOutgoingMessages = !m_outgoingMessages.empty();
        if (hasOutgoingMessages) {
            FD_SET(m_clientSocket, &writefds);
        }
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = hasOutgoingMessages ? 1000 : 100000; // 1ms if sending, 100ms if idle
        
        int result = select(m_clientSocket + 1, &readfds, hasOutgoingMessages ? &writefds : nullptr, nullptr, &timeout);
        
        if (result < 0) {
            if (errno == EINTR) continue; // Interrupted system call, retry
            std::cerr << "Error in select(): " << strerror(errno) << std::endl;
            break;
        }
        
        // Process all pending outgoing messages when socket is writable
        if (result > 0 && hasOutgoingMessages && FD_ISSET(m_clientSocket, &writefds)) {
            // Send multiple messages if available to reduce system calls
            int messagesSent = 0;
            Message msg(static_cast<MessageType>(0), "");
            while (messagesSent < 10 && m_outgoingMessages.tryPop(msg, std::chrono::milliseconds(0))) {
                std::cout << "Sending queued message (batch " << messagesSent + 1 << ")..." << std::endl;
                sendQueuedMessage(msg);
                if (!m_connected) break; // Stop if send failed
                messagesSent++;
            }
        }
        
        // Handle incoming messages
        if (result > 0 && FD_ISSET(m_clientSocket, &readfds)) {
            std::cout << "Data available to read..." << std::endl;
            if (!processIncomingMessage()) {
                std::cout << "Failed to process incoming message, connection lost" << std::endl;
                break;
            }
        }
        
        // Check termination condition
        if (m_terminate || !m_connected) {
            std::cout << "Breaking from network loop: terminate=" << m_terminate << ", connected=" << m_connected << std::endl;
            break;
        }
    }
    
    std::cout << "Network thread ending." << std::endl;
}

std::optional<MessageHeader> SimpleIMClient::readMessageHeader()
{
    if (m_clientSocket <= -1) {
        return std::nullopt;
    }

    char headerBuffer[5];
    size_t totalReceived = 0;
    
    // Since select() confirmed data is available, use blocking recv
    while (totalReceived < sizeof(headerBuffer) && !m_terminate) {
        int bytesReceived = recv(m_clientSocket, headerBuffer + totalReceived, 
                               sizeof(headerBuffer) - totalReceived, 0);
        
        if (bytesReceived > 0) {
            totalReceived += bytesReceived;
        } else if (bytesReceived == 0) {
            std::cout << "Server disconnected." << std::endl;
            m_connected = false;
            return std::nullopt;
        } else {
            std::cerr << __FUNCTION__ << "Error: Could not receive data from server: " << strerror(errno) << std::endl;
            m_connected = false;
            return std::nullopt;
        }
    }

    if (totalReceived != sizeof(headerBuffer)) {
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
    size_t totalReceived = 0;
    
    // Since select() confirmed data is available, use blocking recv
    while (totalReceived < dataLen && !m_terminate) {
        size_t bytesReceived = recv(m_clientSocket, payloadBuffer.data() + totalReceived, 
                                  dataLen - totalReceived, 0);

        if (bytesReceived > 0) {
            totalReceived += bytesReceived;
        } else if (bytesReceived == 0) {
            std::cout << "Server disconnected during payload read." << std::endl;
            m_connected = false;
            return std::string();
        } else {
            std::cerr << __FUNCTION__ << "Error: Could not receive payload from server: " << strerror(errno) << std::endl;
            m_connected = false;
            return std::string();
        }
    }

    if (totalReceived != dataLen) {
        std::cerr << __FUNCTION__ << "Error: Incomplete payload received (" << totalReceived << "/" << dataLen << ")" << std::endl;
        m_connected = false;
        return std::string();
    }

    return std::string(payloadBuffer.begin(), payloadBuffer.end());
}

bool SimpleIMClient::processIncomingMessage()
{
    std::optional<MessageHeader> header = readMessageHeader();
    if (!header.has_value()) {
        return false; // Connection lost or error
    }
    
    if (m_terminate) {
        return false;
    }
    
    if (header->length > 0) {
        std::cout << "Reading message data of length: " << header->length << std::endl;
        std::string data = readMessageData(header->length);
        if (data.empty()) {
            return false; // Failed to read data
        }
        handleReceivedMessage(header->type, data);
    } else {
        // Handle messages with no payload
        std::cout << "Handling message with no payload..." << std::endl;
        handleReceivedMessage(header->type, "");
    }
    
    return true;
}

void SimpleIMClient::handleReceivedMessage(MessageType type, const std::string& data)
{
    std::cout << __FUNCTION__ << " user name: " << m_clientUsername << std::endl;
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
            // Invoke callback if set
            if (m_chatMessageCallback) {
                // Parse username from message format "username: message"
                size_t colonPos = data.find(": ");
                if (colonPos != std::string::npos) {
                    std::string username = data.substr(0, colonPos);
                    std::string message = data.substr(colonPos + 2);
                    m_chatMessageCallback(username, message);
                } else {
                    m_chatMessageCallback("Unknown", data);
                }
            }
            break;
        case MessageType::ChatMessageDM:
            std::cout << "Direct Message: " << data << std::endl;
            // Invoke callback if set
            if (m_chatMessageCallback) {
                // Parse DM format "from_user: message"
                size_t colonPos = data.find(": ");
                if (colonPos != std::string::npos) {
                    std::string username = data.substr(0, colonPos);
                    std::string message = data.substr(colonPos + 2);
                    m_chatMessageCallback(username, message);
                } else {
                    m_chatMessageCallback("Unknown", data);
                }
            }
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
    } else {
        std::cout << "Connected users: " << data << std::endl;
    }
    
    // Invoke callback if set
    if (m_connectedUsersListCallback) {
        m_connectedUsersListCallback(data);
    }
}

void SimpleIMClient::handleClientConnected(const std::string& data)
{
    std::cout << "User '" << data << "' joined the chat." << std::endl;
    
    // Invoke callback if set
    if (m_userConnectedCallback) {
        m_userConnectedCallback(data);
    }
}

void SimpleIMClient::handleClientDisconnected(const std::string& data)
{
    std::cout << "User '" << data << "' left the chat." << std::endl;
    
    // Invoke callback if set
    if (m_userDisconnectedCallback) {
        m_userDisconnectedCallback(data);
    }
}
