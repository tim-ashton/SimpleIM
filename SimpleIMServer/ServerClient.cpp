#include "ServerClient.h"
#include "ClientManager.h"

#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h>

namespace {
constexpr uint32_t kMaxPayloadLength = 1024 * 1024;

bool isValidMessageType(MessageType type)
{
    return type >= MessageType::UserLogon && type <= MessageType::ClientDisconnected;
}

bool recvAll(int socket, char* buffer, size_t length)
{
    size_t totalReceived = 0;
    while (totalReceived < length) {
        const ssize_t bytesReceived = recv(socket, buffer + totalReceived, length - totalReceived, 0);
        if (bytesReceived == 0) {
            errno = 0; // peer performed an orderly shutdown
            return false;
        }

        if (bytesReceived < 0) {
            if (errno == EINTR) {
                continue;
            }

            return false;
        }

        totalReceived += static_cast<size_t>(bytesReceived);
    }

    return true;
}

bool sendAll(int socket, const char* buffer, size_t length)
{
    size_t totalSent = 0;
    while (totalSent < length) {
        const ssize_t bytesSent = send(socket, buffer + totalSent, length - totalSent, 0);
        if (bytesSent < 0) {
            if (errno == EINTR) {
                continue;
            }

            return false;
        }

        if (bytesSent == 0) {
            return false;
        }

        totalSent += static_cast<size_t>(bytesSent);
    }

    return true;
}

}

ServerClient::ServerClient(int socket,
               std::function<void(std::string)> disconnectCallback)
    : m_socket(socket)
    , m_userId("")
    , m_terminate(false)
    , m_state(ConnectionState::PreAuth)
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
    
    // Then wait for thread to finish. If destruction occurs on the same client thread,
    // detach to avoid self-join.
    if (m_thread && m_thread->joinable()) {
        if (m_thread->get_id() == std::this_thread::get_id()) {
            m_thread->detach();
        } else {
            m_thread->join();
        }
    }
}

bool ServerClient::handleLogon(ClientManager* manager)
{
    if (m_state != ConnectionState::PreAuth) {
        std::cerr << __PRETTY_FUNCTION__ << "Logon attempted outside pre-auth state." << std::endl;
        sendMessage(MessageType::LoginFailure, "Already authenticated");
        return false;
    }

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
    m_state = ConnectionState::Authenticated;
    
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
            const std::string userIdSnapshot = m_userId;
            
            while(!m_terminate && m_socket > -1) {

                std::optional<MessageHeader> header = readMessageHeader();

                if(header.has_value() && !m_terminate) {

                    std::string messageData = readMessageData(header->length);

                    if (!m_terminate) {
                        switch(header->type)
                        {
                            case MessageType::UserLogon:
                                std::cout << "User logon during active session ignored for user: " << m_userId << std::endl;
                            break;
                            case MessageType::UserLogoff:
                                m_state = ConnectionState::Closing;
                                handleSocketError();
                                return;
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
                    return;
                }
            }
            
            std::cout << __PRETTY_FUNCTION__ << "Client thread ending for user: " << userIdSnapshot << std::endl;
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
        if (!recvAll(m_socket, headerBuffer, sizeof(headerBuffer))) {
            if (!m_terminate && errno != 0) {
                std::cerr << __PRETTY_FUNCTION__
                          << "Error: Failed to receive complete header from client (errno="
                          << errno << ": " << std::strerror(errno) << ")" << std::endl;
            }
            handleSocketError();
            return std::nullopt;
        }

        const MessageType type = static_cast<MessageType>(headerBuffer[0]);
        uint32_t payloadLength = 0;
        std::memcpy(&payloadLength, &headerBuffer[1], sizeof(uint32_t));
        payloadLength = ntohl(payloadLength);

        if (!isValidMessageType(type)) {
            std::cerr << __PRETTY_FUNCTION__ << "Invalid message type: "
                      << static_cast<int>(headerBuffer[0]) << std::endl;
            handleSocketError();
            return std::nullopt;
        }

        if (payloadLength > kMaxPayloadLength) {
            std::cerr << __PRETTY_FUNCTION__ << "Payload too large: " << payloadLength << std::endl;
            handleSocketError();
            return std::nullopt;
        }

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
    if (dataLen == 0) {
        return std::string();
    }

    if (dataLen > kMaxPayloadLength) {
        std::cerr << __PRETTY_FUNCTION__ << "Payload too large: " << dataLen << std::endl;
        handleSocketError();
        return std::string();
    }

    if (m_socket > -1) {
        std::vector<char> payloadBuffer(dataLen);
        if (!recvAll(m_socket, payloadBuffer.data(), dataLen)) {
            if (!m_terminate) {
                std::cerr << __PRETTY_FUNCTION__ << "Error: Failed to receive complete payload from client" << std::endl;
            }
            handleSocketError();
            return std::string();
        }

        return std::string(payloadBuffer.begin(), payloadBuffer.end());
    }

    std::cerr << __PRETTY_FUNCTION__ << "Invalid socket.";
    return std::string();
}

void ServerClient::handleSocketError()
{
    m_state = ConnectionState::Closing;
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
    const uint32_t networkLength = htonl(header.length);
    std::memcpy(&headerBuffer[1], &networkLength, sizeof(uint32_t));
    
    if (!sendAll(m_socket, headerBuffer, sizeof(headerBuffer))) {
        std::cerr << __PRETTY_FUNCTION__ << "Failed to send header" << std::endl;
        handleSocketError();
        return false;
    }
    
    // Send data if any
    if (!data.empty()) {
        if (!sendAll(m_socket, data.data(), data.length())) {
            std::cerr << __PRETTY_FUNCTION__ << "Failed to send data" << std::endl;
            handleSocketError();
            return false;
        }
    }
    
    return true;
}
