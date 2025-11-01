#pragma once

#include <stdint.h>

enum class MessageType : uint8_t
{
    UserLogon,
    UserLogoff,
    ChatMessageBroadcast,
    ChatMessageDM,
    LoginSuccess,
    LoginFailure,
    ConnectedClientsList,
    ClientConnected,
    ClientDisconnected
};