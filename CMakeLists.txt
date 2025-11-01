cmake_minimum_required(VERSION 3.28)

project(SimpleIM)

add_subdirectory(SimpleIMServer)
add_subdirectory(SimpleIMClient)
add_subdirectory(SimpleIMGuiClient)
add_subdirectory(SimpleIMLib)

add_subdirectory(deps)