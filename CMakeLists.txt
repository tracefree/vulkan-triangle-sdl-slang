cmake_minimum_required(VERSION 3.5)
project(prosper)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
find_package(Vulkan REQUIRED)

add_executable(prosper src/main.cpp)

target_link_libraries(prosper PRIVATE SDL3::SDL3 Vulkan::Vulkan)

