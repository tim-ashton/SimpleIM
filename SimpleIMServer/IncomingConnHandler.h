#pragma once
#include "ClientHandler.h"

#include <thread>
#include <memory>


class IncomingConnHandler 
{

public:
    IncomingConnHandler();
    ~IncomingConnHandler();

    void start();

private:
    ClientHandler m_clientHandler;

    bool m_terminate;
    std::unique_ptr<std::thread> m_acceptorThread;
};