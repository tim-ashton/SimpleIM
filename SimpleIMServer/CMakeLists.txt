cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(SimpleIMServer)

add_executable(${PROJECT_NAME}
    SimpleIMServer.cpp
    Client.h
    Client.cpp
    ClientManager.h
    ClientManager.cpp
    IncomingConnHandler.h
    IncomingConnHandler.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SimpleIMLib
)