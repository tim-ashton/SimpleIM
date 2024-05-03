#pragma once

#include <thread>
#include <memory>


class IncomingConnHandler 
{

public:
    IncomingConnHandler();
    ~IncomingConnHandler();

    void start();

private:
    bool m_terminate;
    std::unique_ptr<std::thread> m_acceptorThread;
};