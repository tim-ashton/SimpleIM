#include "Message.h"

#include <arpa/inet.h>


Message::Message(MessageType messageType, const std::string& data)
    : m_header(messageType, data.size())
    , m_messageData(data)
{}

std::vector<uint8_t> Message::to_bytes() const
{
    std::vector<uint8_t> bytes(1 + 4 + m_messageData.size());

    bytes[0] = static_cast<uint8_t>(m_header.type);
    const uint32_t networkLength = htonl(m_header.length);
    std::memcpy(&bytes[1], &networkLength, sizeof(networkLength));

    // The message
    std::memcpy(&bytes[5], m_messageData.data(), m_messageData.size());
    return bytes;
}

Message Message::from_bytes(const std::vector<uint8_t>& data)
{
    if (data.size() < sizeof(MessageHeader)) 
        std::cout << "Invalid message format received!" << std::endl;

    MessageType type = static_cast<MessageType>(data[0]);

    uint32_t length = 0;
    std::memcpy(&length, &data[1], 4);
    length = ntohl(length);

    std::string payload(data.begin() + sizeof(MessageHeader), 
                        data.begin() + sizeof(MessageHeader) + length);

    return Message(type, payload);
}