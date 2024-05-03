#include "IncomingConnHandler.h"

#include <iostream>
#include <chrono>
#include <thread>




int main(int argc, char *argv[]) {

    std::cout << "Starting SimpleIM Server" << std::endl;
    
    IncomingConnHandler connectionHandler;
    connectionHandler.start();


    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}