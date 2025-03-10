#include <iostream>
#include <string>

#include "SimpleIMClient.h"

int main(int argc, char *argv[]) {

    bool exit = false;
    SimpleIMClient client;
    std::string username;

    std::cout << "Starting SimpleIM Client" << std::endl;
    std::cout << "Enter username: ";
    std::getline (std::cin, username);
    std::cout << "Welcome to Simple IM " << username << ". If you want to exit type \"!!exit\"." << std::endl;
    

    // For now expect just the username as the first message.
    client.logon(username);

    std::string message;
    while(!exit) {
        std::getline (std::cin, message);
        if(message == "!!exit") {
            client.disconnectFromServer();
            exit = true;
            continue;
        }
        client.sendChatMessage(message);
    }

    std::cout << "Goodbye." << std::endl;
    return 0;
}