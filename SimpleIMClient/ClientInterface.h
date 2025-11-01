#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <string>
#include <functional>

class SimpleIMClient;

class ClientInterface
{

public:
    ClientInterface();
    ~ClientInterface();

    bool run();
    void stop();
    bool isRunning() const;
    
    // Set the client instance to work with
    void setClient(SimpleIMClient* client);
    
    // Callback for when the interface should exit
    void setExitCallback(std::function<void()> callback);

private:
    std::unique_ptr<std::thread> m_clientInterfaceThread;
    std::atomic<bool> m_terminate = false;
    
    SimpleIMClient* m_client = nullptr;
    std::function<void()> m_exitCallback;
    
    void interfaceLoop();
    void printHelp();
    void handleCommand(const std::string& command);

};