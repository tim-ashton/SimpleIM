FetchContent_Declare(
    googletest
    GIT_REPOSITORY  https://github.com/google/googletest.git
    GIT_TAG         v1.17.0
    GIT_SHALLOW     TRUE
)
set_directory_properties(PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    SYSTEM TRUE
)

FetchContent_MakeAvailable(googletest)