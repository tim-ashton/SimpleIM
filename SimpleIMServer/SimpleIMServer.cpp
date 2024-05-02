#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>



int main(int argc, char *argv[]) {

    std::cout << "Starting SimpleIM Server" << std::endl;
    
     // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error: Could not create socket\n";
        return 1;
    }

    // Bind the socket to an address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // Port number
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error: Could not bind socket to address\n";
        close(serverSocket);
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error: Could not listen on socket\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Server listening on port 8080...\n";

    // Accept incoming connections
    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
    if (clientSocket == -1) {
        std::cerr << "Error: Could not accept incoming connection\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

    // Receive data from the client
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == -1) {
        std::cerr << "Error: Could not receive data from client\n";
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    // Null terminate the received data
    buffer[bytesReceived] = '\0';

    // Print the received data
    std::cout << "Received data from client: " << buffer << std::endl;

    // Close sockets
    close(clientSocket);
    close(serverSocket);

    return 0;
}