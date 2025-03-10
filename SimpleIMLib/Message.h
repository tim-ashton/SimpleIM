#pragma once

#include <cstring>
#include <iostream>
#include <string>
#include <vector>


#include "MessageHeader.h"

class Message {
public:
    explicit Message(MessageType messageType, const std::string& data);

    std::vector<uint8_t> to_bytes() const;

    static Message from_bytes(const std::vector<uint8_t>& data);

private:
    MessageHeader m_header;
    std::string m_messageData;
};