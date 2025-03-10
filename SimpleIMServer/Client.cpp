#include "Client.h"

#include <iostream>
#include <sys/socket.h>

Client::Client(int socket,
               std::function<void(std::string)> disconnectCallback)
    : m_socket(socket)
    , m_userId("")
    , m_terminate(false)
    , m_clientDisconnected(disconnectCallback)
{}

Client::~Client()
{
    m_terminate = true;
    if(m_thread->joinable()) {
        m_thread->join();
    }
}

std::string Client::handleLogon()
{
    std::optional<MessageHeader> header = readMessageHeader();
    if(header.has_value()) {

        if(header->type != MessageType::UserLogon) {
            std::cerr << __PRETTY_FUNCTION__ << "Incorrecnt login message type." << std::endl;
            return m_userId;
        }

        // currently no limits on the username.
        std::string messageData = readMessageData(header->length);
        m_userId = messageData;
        
    }

    std::cout << __PRETTY_FUNCTION__ << "User ID: " << m_userId << std::endl;
    return m_userId;
}

void Client::run()
{
    if (!m_thread)
    {
        std::cout << __PRETTY_FUNCTION__ << "Client starting..." << std::endl;

        m_thread.reset(new std::thread([this]()->void {
            
            while(!m_terminate) {

                std::optional<MessageHeader> header = readMessageHeader();

                if(header.has_value()) {

                    std::string messageData = readMessageData(header->length);

                    switch(header->type)
                    {
                        case MessageType::UserLogon:
                            std::cout << "User logon during active session." << std::endl;
                            m_userId = messageData;
                        break;
                        case MessageType::UserLogoff:
                        break;
                        case MessageType::ChatMessage:
                            std::cout << "Received chat message: " << messageData << std::endl;
                        break;
                    }

                }
            } }));
    }
    else
    {
        std::cerr << __PRETTY_FUNCTION__  << "Client thread already running. Run called twice!";
    }
}

std::optional<MessageHeader> Client::readMessageHeader()
{
    if (m_socket > -1)
    {
        char headerBuffer[5];
        int bytesReceived = recv(m_socket, headerBuffer, sizeof(headerBuffer), 0);
        if(bytesReceived == 0) {
            handleSocketError();
        }
        else if (bytesReceived == -1)
        {
            std::cerr << __PRETTY_FUNCTION__ << "Error: Could not receive data from client\n";

            handleSocketError();
            return std::nullopt;
        }
        else if (bytesReceived < sizeof(headerBuffer))
        {
            std::cerr << __PRETTY_FUNCTION__ << "Error: Incomplete header received\n";

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

std::string Client::readMessageData(uint32_t dataLen)
{
    if(dataLen < 1){
        std::cout << __PRETTY_FUNCTION__ << "No data.";
        return std::string();
    }

    if (m_socket > -1) {

        std::vector<char> payloadBuffer(dataLen);
        size_t bytesReceived = recv(m_socket, payloadBuffer.data(), dataLen, 0);

        if (bytesReceived == -1) {
            std::cerr << __PRETTY_FUNCTION__ << "Error: Could not receive payload from client" << std::endl;
            
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

void Client::handleSocketError()
{
    m_terminate = true;

    close(m_socket);
    m_socket = -1;

    if(m_clientDisconnected)
        m_clientDisconnected(m_userId);
}
