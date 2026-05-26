#include "IncomingConnHandler.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace {
std::atomic<bool> g_running{true};

void stopServer(int) {
    g_running = false;
}
} // namespace

int main(int argc, char *argv[]) {
    std::signal(SIGINT, stopServer);
    std::signal(SIGTERM, stopServer);

    std::cout << "Starting SimpleIM Server" << std::endl;

    IncomingConnHandler connectionHandler;
    connectionHandler.start();

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    connectionHandler.stop();

    return 0;
}
