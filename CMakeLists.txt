cmake_minimum_required(VERSION 3.28)

project(SimpleIM)

include(CTest)

add_subdirectory(SimpleIMServer)
add_subdirectory(SimpleIMClient)
add_subdirectory(SimpleIMGuiClient)
add_subdirectory(SimpleIMLib)

add_subdirectory(deps)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()
