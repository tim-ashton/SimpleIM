cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(SimpleIMTests)

add_executable(${PROJECT_NAME}
    main.cpp
    TestServerClient.cpp
    ../SimpleIMServer/ServerClient.cpp
    ../SimpleIMServer/ClientManager.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../SimpleIMServer
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SimpleIMLib
    GTest::gtest
)

add_test(NAME TestServerClient COMMAND ${PROJECT_NAME})
