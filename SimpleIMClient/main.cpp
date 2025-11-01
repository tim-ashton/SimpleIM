#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <SimpleIMClient.h>
#include "ClientInterface.h"

int main(int argc, char *argv[]) {

    SimpleIMClient client;
    ClientInterface interface;
    std::string username;

    std::cout << "=== Starting SimpleIM Client ===" << std::endl;
    std::cout << "Enter username: ";
    
    if (!std::getline(std::cin, username) || username.empty()) {
        std::cout << "Invalid username. Exiting." << std::endl;
        return 1;
    }
    
    std::cout << "Connecting to SimpleIM Server..." << std::endl;

    // Attempt to log in
    client.logon(username);
    
    // Give some time for login to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    if (!client.connected()) {
        std::cout << "Failed to connect to server. Exiting." << std::endl;
        return 1;
    }

    std::cout << "✓ Successfully connected as '" << username << "'" << std::endl;

    // Set up the interface
    interface.setClient(&client);
    interface.setExitCallback([&client]() {
        client.disconnectFromServer();
    });

    // Start the interactive interface
    if (!interface.run()) {
        std::cout << "Failed to start client interface." << std::endl;
        client.disconnectFromServer();
        return 1;
    }

    // Wait for the interface to finish naturally (when user quits)
    // The interface will stop itself when the user types quit/exit
    while (interface.isRunning() && client.connected()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Clean shutdown
    interface.stop();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}