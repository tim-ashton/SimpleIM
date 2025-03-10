#pragma once

#include "MessageType.h"

struct MessageHeader 
{
    MessageType type;
    uint32_t length;

    MessageHeader(MessageType messageType, uint32_t messageLength) 
        : type(messageType)
        , length(messageLength)
    {}
};