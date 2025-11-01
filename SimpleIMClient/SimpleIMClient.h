#pragma once

#include <Message.h>
#include <thread>
#include <memory>
#include <functional>
#include <optional>

class SimpleIMClient
{
public:
    SimpleIMClient();
    ~SimpleIMClient();

    bool connected();

    void logon(const std::string &username);

    void sendChatMessage(const std::string &message);

    void disconnectFromServer();

private:
    int m_clientSocket = -1;
    bool m_connected = false;
    bool m_terminate = false;
    std::unique_ptr<std::thread> m_receiveThread;

    bool connectToServer();
    void sendMessage(const Message& message);
    
    // Message receiving functionality
    void startReceiveThread();
    void receiveMessages();
    std::optional<MessageHeader> readMessageHeader();
    std::string readMessageData(uint32_t dataLen);
    void handleReceivedMessage(MessageType type, const std::string& data);
    
    // Message handlers
    void handleLoginSuccess(const std::string& data);
    void handleLoginFailure(const std::string& data);
    void handleConnectedClientsList(const std::string& data);
    void handleClientConnected(const std::string& data);
    void handleClientDisconnected(const std::string& data);
};