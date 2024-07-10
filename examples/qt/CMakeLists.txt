cmake_minimum_required(VERSION 3.0.0)

project(NtfyToastCliQtExample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

# if  build as part of ntfytoast project we don't look it up
if (NOT TARGET NtfyToast::NtfyToastActions)
    find_package(LibNtfyToast REQUIRED)
endif()

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Network NtfyToast::NtfyToastActions)
