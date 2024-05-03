cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(SimpleIMServer)

add_executable(${PROJECT_NAME}
    SimpleIMServer.cpp
    IncomingConnHandler.h
    IncomingConnHandler.cpp
)