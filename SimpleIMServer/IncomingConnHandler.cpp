#include "IncomingConnHandler.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

IncomingConnHandler::IncomingConnHandler()
{
}

IncomingConnHandler::~IncomingConnHandler()
{
}

void IncomingConnHandler::start()
{
    if(!m_acceptorThread) {
        std::cout << "IncomingConnHandler starting..." << std::endl;

        m_acceptorThread.reset(new std::thread([&terminate = this->m_terminate]()->void {

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

            while (!terminate) {

                sockaddr_in clientAddr;
                socklen_t clientAddrSize = sizeof(clientAddr);
                int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
                if (clientSocket == -1) {
                    std::cerr << "Error: Could not accept incoming connection\n";
                    continue;
                }

                std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

                // TODO
                // Spawn a new thread to handle the connection
                // std::thread workerThread(handleConnection, clientSocket);
                // workerThread.detach(); // Detach the thread, allowing it to run independently

                {
                    // Receive data from the client
                    char buffer[1024];
                    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
                    if (bytesReceived == -1) {
                        std::cerr << "Error: Could not receive data from client\n";
                        close(clientSocket);
                        close(serverSocket);
                        return;
                    }

                    // Null terminate the received data
                    buffer[bytesReceived] = '\0';

                    // Print the received data
                    std::cout << "Received data from client: " << buffer << std::endl;
                    close(clientSocket);
                }
            }

            close(serverSocket);

        }));       
    }
}
