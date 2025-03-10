#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include <string>

#include "SimpleIMClient.h"


SimpleIMClient::SimpleIMClient()
{}

SimpleIMClient::~SimpleIMClient()
{
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
    sendMessage(Message(MessageType::UserLogon, username));
}

void SimpleIMClient::sendChatMessage(const std::string &message)
{
    sendMessage(Message(MessageType::ChatMessage, message));  // Add destination user.
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
        close(m_clientSocket);
        m_connected = false;
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
