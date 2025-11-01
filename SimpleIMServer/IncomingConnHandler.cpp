#include "IncomingConnHandler.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

IncomingConnHandler::IncomingConnHandler()
    : m_terminate(false)
{}

IncomingConnHandler::~IncomingConnHandler()
{
    stop();
}

void IncomingConnHandler::stop()
{
    m_terminate = true;
    if(m_acceptorThread && m_acceptorThread->joinable()) {
        m_acceptorThread->join();
        m_acceptorThread.reset();
    }
}

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

                fd_set readfds;
                struct timeval timeout;
                
                FD_ZERO(&readfds);
                FD_SET(serverSocket, &readfds);
                
                timeout.tv_sec = 0;
                timeout.tv_usec = 10000;
                
                int selectResult = select(serverSocket + 1, &readfds, NULL, NULL, &timeout);
                
                if (selectResult == -1) {
                    if (errno != EINTR) {
                        std::cerr << "Error: select() failed\n";
                        break;
                    }
                    // signal interrupt
                    continue;
                }
                else if (selectResult == 0) {
                    // timeout
                    continue;
                }
                else if (FD_ISSET(serverSocket, &readfds)) {
                    // Socket is ready for accept
                    sockaddr_in clientAddr;
                    socklen_t clientAddrSize = sizeof(clientAddr);
                    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
                    
                    if (clientSocket == -1) {
                        if (errno != EWOULDBLOCK && errno != EAGAIN) {
                            std::cerr << "Error: Could not accept incoming connection\n";
                        }
                        continue;
                    }

                    std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
                    m_clientManager.addConnectedClient(clientSocket);
                }
            }

            close(serverSocket);
            std::cout << "IncomingConnHandler exiting." << std::endl;
        }));       
    }
}
