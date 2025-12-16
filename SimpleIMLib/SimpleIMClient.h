#pragma once

#include <Message.h>
#include <MessageQueue.h>
#include <thread>
#include <memory>
#include <functional>
#include <optional>

class SimpleIMClient
{
public:
    // Callback types for UI notifications
    using UserConnectedCallback = std::function<void(const std::string& username)>;
    using UserDisconnectedCallback = std::function<void(const std::string& username)>;
    using ConnectedUsersListCallback = std::function<void(const std::string& userList)>;
    using ChatMessageCallback = std::function<void(const std::string& username, const std::string& message)>;
    
    SimpleIMClient();
    ~SimpleIMClient();

    bool connected();

    void logon(const std::string &username);

    void sendChatMessage(const std::string &message);
    void sendDirectMessage(const std::string &targetUsername, const std::string &message);

    void disconnectFromServer();
    
    // Callback setters
    void setUserConnectedCallback(UserConnectedCallback callback) { m_userConnectedCallback = callback; }
    void setUserDisconnectedCallback(UserDisconnectedCallback callback) { m_userDisconnectedCallback = callback; }
    void setConnectedUsersListCallback(ConnectedUsersListCallback callback) { m_connectedUsersListCallback = callback; }
    void setChatMessageCallback(ChatMessageCallback callback) { m_chatMessageCallback = callback; }

private:
    int m_clientSocket = -1;
    bool m_connected = false;
    bool m_terminate = false;
    std::string m_clientUsername;
    std::unique_ptr<std::thread> m_networkThread;
    
    // Message queue for outgoing messages
    MessageQueue m_outgoingMessages;
    
    // UI callback functions
    UserConnectedCallback m_userConnectedCallback;
    UserDisconnectedCallback m_userDisconnectedCallback;
    ConnectedUsersListCallback m_connectedUsersListCallback;
    ChatMessageCallback m_chatMessageCallback;

    bool connectToServer();
    void queueMessage(const Message& message);
    void sendQueuedMessage(const Message& message);
    
    // Network thread functionality
    void startNetworkThread();
    void networkLoop();
    
    // Message receiving functionality
    std::optional<MessageHeader> readMessageHeader();
    std::string readMessageData(uint32_t dataLen);
    bool processIncomingMessage();
    void handleReceivedMessage(MessageType type, const std::string& data);
    
    // Message handlers
    void handleLoginSuccess(const std::string& data);
    void handleLoginFailure(const std::string& data);
    void handleConnectedClientsList(const std::string& data);
    void handleClientConnected(const std::string& data);
    void handleClientDisconnected(const std::string& data);
};