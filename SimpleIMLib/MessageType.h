#pragma once

#include <stdint.h>


enum class MessageType : uint8_t
{
    UserLogon,
    UserLogoff,
    ChatMessage
};