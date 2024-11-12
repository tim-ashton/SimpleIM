#pragma once

#include <cstring>
#include <iostream>
#include <string>
#include <vector>


#include "MessageType.h"

class Message {

public:
    MessageType m_messageType;
    std::string m_messageData;

    explicit Message(MessageType messageType, const std::string& data);

    std::vector<uint8_t> to_bytes() const;

    // Convert to message.
    static Message from_bytes(const std::vector<uint8_t>& data);
};