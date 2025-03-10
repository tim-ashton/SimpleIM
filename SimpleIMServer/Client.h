#pragma once

#include <string>
#include <thread>
#include <memory>
#include <optional>
#include <functional>

#include <Message.h>

class Client 
{
public:
    explicit Client(int socket, 
                    std::function<void(std::string)> disconnectCallback);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    
    std::string handleLogon();
    void run();

private:
    int m_socket;
    std::string m_userId;

    bool m_terminate;
    std::unique_ptr<std::thread> m_thread;

    std::function<void(std::string)> m_clientDisconnected;

    std::optional<MessageHeader> readMessageHeader();
    std::string readMessageData(uint32_t dataLen);

    void handleSocketError();

};