# CMakeList.txt : CMake project for StopRightNow, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

if (WIN32)
	set(PLATFORM_SPECIFIC_SOURCES "main_windows.cpp")
elseif (UNIX)
	set(PLATFORM_SPECIFIC_SOURCES "main_linux.cpp")
endif()

# Add source to this project's executable.
add_executable (StopRightNow "StopRightNow.cpp" "StopRightNow.h" "main.cpp" ${PLATFORM_SPECIFIC_SOURCES})

# TODO: Add tests and install targets if needed.
