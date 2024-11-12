#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include <string>

#include "SimpleIMClient.h"


SimpleIMClient::SimpleIMClient()
{
    if(!connectToServer()) {
        std::cout << "Unable to connect to SimpleIM Server. Exiting." << std::endl;
        return;
    }
}

SimpleIMClient::~SimpleIMClient()
{
}

bool SimpleIMClient::connected()
{
    return m_connected;
}

void SimpleIMClient::logon(const std::string &username)
{
    sendMessage(Message(MessageType::UserLogon, username));
}

bool SimpleIMClient::connectToServer()
{
    std::cout << "SimpleIMClient::connectToServer()" << std::endl;

    m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_clientSocket == -1) {
        std::cerr << "SimpleIMClient::connectToServer() Error: Could not create socket\n";
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8989);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(m_clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "SimpleIMClient::connectToServer() Error: Could not connect to server\n";
        close(m_clientSocket);
        return false;
    }

    std::cout << "SimpleIMClient::connectToServer: connection successful" << std::endl;
    m_connected = true;
    return m_connected;
}

void SimpleIMClient::disconnectFromServer()
{
    if(m_connected)
        close(m_clientSocket);
}

void SimpleIMClient::sendMessage(const Message& message)
{
    std::cout << "SimpleIMClient::sendMessage()" << std::endl;

    if(!m_connected) {
        std::cerr << "SimpleIMClient::sendData() Warning: not connected. Cannot send." << std::endl;
        return;
    }

    std::vector<uint8_t> data = message.to_bytes();

    if (send(m_clientSocket, data.data(), data.size(), 0) == -1) {
        std::cerr << "SimpleIMClient::sendData() Error: Could not send to server." << std::endl;

        // TODO what? Fix this.
        m_connected = false;
        close(m_clientSocket);
        return;
    }
}
