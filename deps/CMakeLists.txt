include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Set CMake policies manually
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)  # Allow option()'s to be set with normal variables
endif()

set(CMAKE_POLICY_DEFAULT_CMP0099 NEW)
if(POLICY CMP0099)
    cmake_policy(SET CMP0099 NEW)  # Make INTERFACE_* link properties transitive
endif()

set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)  # Prefer to set timestamp of files extracted from downloaded archives to extraction time if available
endif()

add_subdirectory(lvgl)
# add_subdirectory(gtest) //Add me.
