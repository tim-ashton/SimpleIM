#include "ClientInterface.h"
#include "SimpleIMClient.h"

#include <iostream>
#include <sstream>
#include <algorithm>

ClientInterface::ClientInterface()
{}

ClientInterface::~ClientInterface()
{
    stop();
}

void ClientInterface::setClient(SimpleIMClient* client)
{
    m_client = client;
}

void ClientInterface::setExitCallback(std::function<void()> callback)
{
    m_exitCallback = callback;
}

bool ClientInterface::run()
{
    if(!m_clientInterfaceThread) {
        m_clientInterfaceThread.reset(new std::thread([this] ()->void {
            interfaceLoop();
        }));
        return true;
    }
    return false;
}

void ClientInterface::stop()
{
    m_terminate = true;
    if(m_clientInterfaceThread && m_clientInterfaceThread->joinable()) {
        m_clientInterfaceThread->join();
        m_clientInterfaceThread.reset();
    }
}

bool ClientInterface::isRunning() const
{
    return m_clientInterfaceThread && !m_terminate;
}

void ClientInterface::interfaceLoop()
{
    std::cout << "\n=== SimpleIM Client Interface ===" << std::endl;
    std::cout << "Type 'help' for available commands" << std::endl;
    
    std::string input;
    while(!m_terminate && m_client && m_client->connected()) {
        std::cout << "SimpleIM> ";
        
        if(!std::getline(std::cin, input)) {
            // EOF reached
            break;
        }
        
        // Trim whitespace
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);
        
        if(input.empty()) {
            continue;
        }
        
        handleCommand(input);
    }
    
    std::cout << "Interface shutting down..." << std::endl;
}

void ClientInterface::printHelp()
{
    std::cout << "\n=== Available Commands ===" << std::endl;
    std::cout << "  help                 - Show this help message" << std::endl;
    std::cout << "  msg <message>        - Send a chat message" << std::endl;
    std::cout << "  say <message>        - Send a chat message (alias for msg)" << std::endl;
    std::cout << "  dm <user> <message>  - Send direct message (@:user format)" << std::endl;
    std::cout << "  status               - Show connection status" << std::endl;
    std::cout << "  quit, exit, bye      - Disconnect and exit" << std::endl;
    std::cout << "  clear                - Clear the screen" << std::endl;
    std::cout << "\nNote: Users joining/leaving will be shown automatically" << std::endl;
    std::cout << "Direct message format: @:username message" << std::endl;
    std::cout << "========================\n" << std::endl;
}

void ClientInterface::handleCommand(const std::string& command)
{
    if(!m_client) {
        std::cout << "Error: No client connection available." << std::endl;
        return;
    }
    
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    // Convert to lowercase for case-insensitive commands
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    
    if(cmd == "help" || cmd == "h" || cmd == "?") {
        printHelp();
    }
    else if(cmd == "msg" || cmd == "say") {
        std::string message;
        std::getline(iss, message);
        // Remove leading space
        if(!message.empty() && message[0] == ' ') {
            message = message.substr(1);
        }
        
        if(message.empty()) {
            std::cout << "Usage: msg <your message>" << std::endl;
        } else {
            m_client->sendChatMessage(message);
            std::cout << "Message sent: " << message << std::endl;
        }
    }
    else if(cmd == "dm") {
        std::string username, message;
        iss >> username;
        std::getline(iss, message);
        // Remove leading space
        if(!message.empty() && message[0] == ' ') {
            message = message.substr(1);
        }
        
        if(username.empty() || message.empty()) {
            std::cout << "Usage: dm <username> <message>" << std::endl;
        } else {
            std::string directMessage = "@:" + username + " " + message;
            m_client->sendChatMessage(directMessage);
            std::cout << "Direct message sent to " << username << ": " << message << std::endl;
        }
    }
    else if(cmd == "status") {
        if(m_client->connected()) {
            std::cout << "Status: Connected to SimpleIM Server" << std::endl;
        } else {
            std::cout << "Status: Not connected" << std::endl;
        }
    }
    else if(cmd == "quit" || cmd == "exit" || cmd == "bye") {
        std::cout << "Disconnecting..." << std::endl;
        m_terminate = true;
        if(m_exitCallback) {
            m_exitCallback();
        }
    }
    else if(cmd == "clear" || cmd == "cls") {
        // Clear screen (works on most terminals)
        std::cout << "\033[2J\033[1;1H" << std::endl;
    }
    else {
        // If it doesn't match any command, treat it as a message
        if(!command.empty()) {
            m_client->sendChatMessage(command);
            std::cout << "Message sent: " << command << std::endl;
        }
    }
}
