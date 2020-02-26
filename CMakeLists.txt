cmake_minimum_required (VERSION 2.6)

project (udp2tcp-cpp)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)
find_package(Threads REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} Threads::Threads)
