#include "Message.h"


Message::Message(MessageType messageType, const std::string& data)
    : m_messageType(messageType)
    , m_messageData(data)
{}

std::vector<uint8_t> Message::to_bytes() const
{
    std::vector<uint8_t> bytes(1 + 4 + m_messageData.size());

    bytes[0] = static_cast<uint8_t>(m_messageType);

    uint32_t messageDataLength = m_messageData.size();
    std::memcpy(&bytes[1], &messageDataLength, 4);

    // The message
    std::memcpy(&bytes[5], m_messageData.data(), m_messageData.size());
    return bytes;
}

Message Message::from_bytes(const std::vector<uint8_t>& data)
{
    if (data.size() < 5) 
        std::cout << "Invalid message format received!" << std::endl;

    MessageType type = static_cast<MessageType>(data[0]);

    uint32_t length = 0;
    std::memcpy(&length, &data[1], 4);

    std::string payload(data.begin() + 5, data.begin() + 5 + length);

    return Message(type, payload);
}