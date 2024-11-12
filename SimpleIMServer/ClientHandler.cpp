#include "ClientHandler.h"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

#include <Message.h>

ClientHandler::ClientHandler()
{
}

ClientHandler::~ClientHandler()
{
}

void ClientHandler::addConnectedClient(int clientSock)
{

    char headerBuffer[5];
    int bytesReceived = recv(clientSock, headerBuffer, sizeof(headerBuffer), 0);
    if (bytesReceived == -1) {
        std::cerr << "Error: Could not receive data from client\n";
        close(clientSock);
        close(clientSock);
        return;
    }
    else if (bytesReceived < sizeof(headerBuffer)) {
        std::cerr << "Error: Incomplete header received\n";
        close(clientSock);
        return;
    }

    MessageType type = static_cast<MessageType>(headerBuffer[0]);
    uint32_t payloadLength = 0;
    std::memcpy(&payloadLength, &headerBuffer[1], sizeof(uint32_t));

    std::vector<char> payloadBuffer(payloadLength);
    bytesReceived = recv(clientSock, payloadBuffer.data(), payloadLength, 0);

    if (bytesReceived == -1) {
        std::cerr << "Error: Could not receive payload from client\n";
        close(clientSock);
        return;
    } else if (bytesReceived < payloadLength) {
        std::cerr << "Error: Incomplete payload received\n";
        close(clientSock);
        return;
    }

    std::string payload(payloadBuffer.begin(), payloadBuffer.end());
    Message message(type, payload);

    // Print the message contents (for debugging purposes)
    std::cout << "Received message type: " << static_cast<int>(message.m_messageType) << std::endl;
    std::cout << "Payload: " << message.m_messageData << std::endl;
    
    // TODO - everything else
    close(clientSock);

}
