cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(SimpleIMClient)

add_executable(${PROJECT_NAME}
    main.cpp
    SimpleIMClient.h
    SimpleIMClient.cpp

    ClientInterface.h
    ClientInterface.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SimpleIMLib
)