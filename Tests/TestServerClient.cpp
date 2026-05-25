#include <gtest/gtest.h>

#include "ClientManager.h"
#include "Message.h"
#include "ServerClient.h"

#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <optional>
#include <vector>
#include <cstring>
#include <atomic>
#include <chrono>
#include <thread>
#include <arpa/inet.h>

class TestServerClient : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, m_sockets), 0);
    }

    void TearDown() override
    {
        if (m_sockets[0] != -1) {
            close(m_sockets[0]);
            m_sockets[0] = -1;
        }

        if (m_sockets[1] != -1) {
            close(m_sockets[1]);
            m_sockets[1] = -1;
        }
    }

    bool sendRaw(const std::vector<uint8_t>& bytes)
    {
        size_t offset = 0;
        while (offset < bytes.size()) {
            const ssize_t sent = send(m_sockets[1], bytes.data() + offset, bytes.size() - offset, 0);
            if (sent <= 0) {
                return false;
            }

            offset += static_cast<size_t>(sent);
        }

        return true;
    }

    std::vector<uint8_t> buildRawMessage(MessageType type, const std::string& payload)
    {
        std::vector<uint8_t> bytes(5 + payload.size());
        bytes[0] = static_cast<uint8_t>(type);
        const uint32_t networkLength = htonl(static_cast<uint32_t>(payload.size()));
        std::memcpy(&bytes[1], &networkLength, sizeof(networkLength));
        std::memcpy(&bytes[5], payload.data(), payload.size());
        return bytes;
    }

    std::optional<MessageHeader> recvHeader()
    {
        char headerBuffer[5];
        const ssize_t received = recv(m_sockets[1], headerBuffer, sizeof(headerBuffer), 0);
        if (received != static_cast<ssize_t>(sizeof(headerBuffer))) {
            return std::nullopt;
        }

        MessageType type = static_cast<MessageType>(headerBuffer[0]);
        uint32_t payloadLength = 0;
        std::memcpy(&payloadLength, &headerBuffer[1], sizeof(uint32_t));
        payloadLength = ntohl(payloadLength);
        return MessageHeader(type, payloadLength);
    }

    int m_sockets[2] = {-1, -1};
};


TEST(TestMessage, ToBytesUsesNetworkByteOrderLength)
{
    Message message(MessageType::UserLogon, "alice");
    std::vector<uint8_t> bytes = message.to_bytes();

    ASSERT_EQ(bytes.size(), 10U);

    uint32_t length = 0;
    std::memcpy(&length, &bytes[1], sizeof(length));
    EXPECT_EQ(ntohl(length), 5U);
}

TEST_F(TestServerClient, HandleLogonReadsSplitHeaderAndPayload)
{
    bool disconnectedCalled = false;
    std::string disconnectedUserId;

    ServerClient client(
        m_sockets[0],
        [&](std::string userId) {
            disconnectedCalled = true;
            disconnectedUserId = userId;
        });

    ClientManager manager;

    std::vector<uint8_t> bytes = buildRawMessage(MessageType::UserLogon, "alice");

    ASSERT_EQ(send(m_sockets[1], bytes.data(), 2, 0), 2);
    ASSERT_EQ(send(m_sockets[1], bytes.data() + 2, 3, 0), 3);
    ASSERT_EQ(send(m_sockets[1], bytes.data() + 5, 2, 0), 2);
    ASSERT_EQ(send(m_sockets[1], bytes.data() + 7, bytes.size() - 7, 0),
              static_cast<ssize_t>(bytes.size() - 7));

    ASSERT_TRUE(client.handleLogon(&manager));
    EXPECT_EQ(client.getUserId(), "alice");
    EXPECT_FALSE(disconnectedCalled);
    EXPECT_TRUE(disconnectedUserId.empty());
}

TEST_F(TestServerClient, HandleLogonFailsWhenPayloadDisconnectsMidMessage)
{
    bool disconnectedCalled = false;
    std::string disconnectedUserId;

    ServerClient client(
        m_sockets[0],
        [&](std::string userId) {
            disconnectedCalled = true;
            disconnectedUserId = userId;
        });

    ClientManager manager;

    std::vector<uint8_t> bytes = buildRawMessage(MessageType::UserLogon, "alice");

    ASSERT_EQ(send(m_sockets[1], bytes.data(), 5, 0), 5);
    ASSERT_EQ(send(m_sockets[1], bytes.data() + 5, 2, 0), 2);

    close(m_sockets[1]);
    m_sockets[1] = -1;

    EXPECT_FALSE(client.handleLogon(&manager));
    EXPECT_TRUE(disconnectedCalled);
    EXPECT_TRUE(disconnectedUserId.empty());
}

TEST_F(TestServerClient, HandleLogonSendsExpectedSuccessMessages)
{
    ServerClient client(m_sockets[0], [](const std::string&) {});
    ClientManager manager;

    ASSERT_TRUE(sendRaw(buildRawMessage(MessageType::UserLogon, "alice")));

    ASSERT_TRUE(client.handleLogon(&manager));

    std::optional<MessageHeader> firstHeader = recvHeader();
    ASSERT_TRUE(firstHeader.has_value());
    EXPECT_EQ(firstHeader->type, MessageType::LoginSuccess);
    EXPECT_EQ(firstHeader->length, strlen("Login successful"));

    std::vector<char> firstPayload(firstHeader->length);
    ASSERT_EQ(recv(m_sockets[1], firstPayload.data(), firstPayload.size(), 0),
              static_cast<ssize_t>(firstPayload.size()));

    std::optional<MessageHeader> secondHeader = recvHeader();
    ASSERT_TRUE(secondHeader.has_value());
    EXPECT_EQ(secondHeader->type, MessageType::ConnectedClientsList);
    EXPECT_EQ(secondHeader->length, 0U);
}

TEST_F(TestServerClient, RunIgnoresRelogonAfterAuthentication)
{
    ServerClient client(m_sockets[0], [](const std::string&) {});
    ClientManager manager;

    ASSERT_TRUE(sendRaw(buildRawMessage(MessageType::UserLogon, "alice")));
    ASSERT_TRUE(client.handleLogon(&manager));
    EXPECT_EQ(client.getUserId(), "alice");

    // Drain handleLogon responses from socket.
    std::optional<MessageHeader> firstHeader = recvHeader();
    ASSERT_TRUE(firstHeader.has_value());
    std::vector<char> firstPayload(firstHeader->length);
    ASSERT_EQ(recv(m_sockets[1], firstPayload.data(), firstPayload.size(), 0),
              static_cast<ssize_t>(firstPayload.size()));
    std::optional<MessageHeader> secondHeader = recvHeader();
    ASSERT_TRUE(secondHeader.has_value());

    client.run(&manager);

    ASSERT_TRUE(sendRaw(buildRawMessage(MessageType::UserLogon, "mallory")));
    ASSERT_TRUE(sendRaw(buildRawMessage(MessageType::UserLogoff, "")));

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(client.getUserId(), "alice");
}

TEST_F(TestServerClient, UserLogoffTriggersDisconnectCallbackExactlyOnce)
{
    std::atomic<int> callbackCount(0);
    std::string disconnectedUserId;

    ServerClient client(
        m_sockets[0],
        [&](std::string userId) {
            ++callbackCount;
            disconnectedUserId = userId;
        });
    ClientManager manager;

    ASSERT_TRUE(sendRaw(buildRawMessage(MessageType::UserLogon, "alice")));
    ASSERT_TRUE(client.handleLogon(&manager));

    // Drain handleLogon responses from socket.
    std::optional<MessageHeader> firstHeader = recvHeader();
    ASSERT_TRUE(firstHeader.has_value());
    std::vector<char> firstPayload(firstHeader->length);
    ASSERT_EQ(recv(m_sockets[1], firstPayload.data(), firstPayload.size(), 0),
              static_cast<ssize_t>(firstPayload.size()));
    std::optional<MessageHeader> secondHeader = recvHeader();
    ASSERT_TRUE(secondHeader.has_value());

    client.run(&manager);
    ASSERT_TRUE(sendRaw(buildRawMessage(MessageType::UserLogoff, "")));

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(callbackCount.load(), 1);
    EXPECT_EQ(disconnectedUserId, "alice");
}
