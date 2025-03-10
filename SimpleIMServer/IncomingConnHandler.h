#pragma once
#include "ClientManager.h"

#include <thread>
#include <memory>


class IncomingConnHandler 
{

public:
    IncomingConnHandler();
    ~IncomingConnHandler();

    void start();

private:
    ClientManager m_clientManager;

    bool m_terminate;
    std::unique_ptr<std::thread> m_acceptorThread;
};