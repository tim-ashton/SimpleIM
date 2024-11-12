#include "IncomingConnHandler.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

IncomingConnHandler::IncomingConnHandler()
{}

IncomingConnHandler::~IncomingConnHandler()
{}

void IncomingConnHandler::start()
{
    if(!m_acceptorThread) {
        std::cout << "IncomingConnHandler starting..." << std::endl;

        m_acceptorThread.reset(new std::thread([this]()->void {

            int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket == -1) {
                std::cerr << "Error: Could not create socket\n";
                return;
            }

            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8989);
            serverAddr.sin_addr.s_addr = INADDR_ANY;

            if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
                std::cerr << "Error: Could not bind socket to address\n";
                close(serverSocket);
                return;
            }

            if (listen(serverSocket, 10) == -1) {
                std::cerr << "Error: Could not listen on socket\n";
                close(serverSocket);
                return;
            }

            std::cout << "Server listening on port 8989...\n";

            while (!m_terminate) {

                sockaddr_in clientAddr;
                socklen_t clientAddrSize = sizeof(clientAddr);
                int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
                if (clientSocket == -1) {
                    std::cerr << "Error: Could not accept incoming connection\n";
                    continue;
                }

                std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
                m_clientHandler.addConnectedClient(clientSocket);
            }

            close(serverSocket);

        }));       
    }
}
