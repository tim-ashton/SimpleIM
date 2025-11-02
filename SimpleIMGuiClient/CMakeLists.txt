
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(SimpleIMGuiClient)

set(header_files
    LoginDialog.h
)

add_executable(${PROJECT_NAME}
    ${header_files}
    LoginDialog.cpp
    main.cpp
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        SimpleIMLib
        lvgl::lvgl
)