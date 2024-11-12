#pragma once

#include <atomic>
#include <memory>
#include <thread>


class ClientInterface
{

public:
    ClientInterface();
    ~ClientInterface();

    bool run();
    void stop();

private:
    std::unique_ptr<std::thread> m_clientInterfaceThread;
    std::atomic<bool> m_terminate = false;

};