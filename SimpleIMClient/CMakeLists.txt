cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(SimpleIMClient)

add_executable(${PROJECT_NAME}
    SimpleIMClient.cpp
)