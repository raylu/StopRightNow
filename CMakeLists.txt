# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("StopRightNow")

if (WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif (UNIX)
    add_definitions(-DPLATFORM_LINUX)
endif()


# Include sub-projects.
add_subdirectory ("StopRightNow")
