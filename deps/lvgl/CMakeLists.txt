set(LV_CONF_BUILD_DISABLE_DEMOS ON)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY  https://github.com/lvgl/lvgl.git
    GIT_TAG         v9.4.0
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
set_directory_properties(PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    SYSTEM TRUE
)

# Ensure LV_BUILD_CONF_PATH is an absolute path
if(DEFINED LV_BUILD_CONF_PATH)
    if(NOT IS_ABSOLUTE "${LV_BUILD_CONF_PATH}")
        # Convert relative path to absolute using binary directory as base
        get_filename_component(LV_BUILD_CONF_PATH "${LV_BUILD_CONF_PATH}" ABSOLUTE BASE_DIR "${CMAKE_BINARY_DIR}")
        message(STATUS "Converted LV_BUILD_CONF_PATH to absolute: ${LV_BUILD_CONF_PATH}")
    endif()
elseif()
    message(FATAL_ERROR "LV_BUILD_CONF_PATH not defined")
endif()

FetchContent_MakeAvailable(lvgl)

find_package(SDL2 REQUIRED)
target_link_libraries(lvgl
    PRIVATE
        SDL2::SDL2
)

