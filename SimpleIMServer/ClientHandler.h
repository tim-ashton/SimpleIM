#pragma once

class ClientHandler
{
public:
    ClientHandler();
    ~ClientHandler();

    void addConnectedClient(int clientSock);
};