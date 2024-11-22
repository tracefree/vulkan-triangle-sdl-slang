cmake_minimum_required(VERSION 3.5)
project(prosper)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY build/bin)

find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
find_package(Vulkan REQUIRED)

add_executable(prosper
    src/main.cpp
    src/renderer.cpp
)
target_include_directories(prosper PRIVATE src)

target_link_libraries(prosper PRIVATE SDL3::SDL3 Vulkan::Vulkan)

