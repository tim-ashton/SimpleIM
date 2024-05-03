#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>


void sendHello() {
    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error: Could not create socket\n";
        return;
    }

    // Server address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8989); // Port number
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP address of the server

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error: Could not connect to server\n";
        close(clientSocket);
        return;
    }

    // Send "Hello" message
    const char* message = "Hello";
    if (send(clientSocket, message, strlen(message), 0) == -1) {
        std::cerr << "Error: Could not send message to server\n";
        close(clientSocket);
        return;
    }

    std::cout << "Sent: " << message << std::endl;

    // Close the connection
    close(clientSocket);
}

int main(int argc, char *argv[]) {

    std::cout << "Starting SimpleIM Client" << std::endl;
    sendHello();
  
    return 0;
}