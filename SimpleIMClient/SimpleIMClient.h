#pragma once

#include <Message.h>

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

    bool connectToServer();
    void sendMessage(const Message& message);
};