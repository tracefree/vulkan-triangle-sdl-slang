cmake_minimum_required(VERSION 3.5)
project(vulkan-triangle)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY build/bin)

find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
find_package(Vulkan REQUIRED)

add_executable(vulkan-triangle
    src/main.cpp
    src/renderer.cpp
)
target_include_directories(vulkan-triangle PRIVATE src)

target_link_libraries(vulkan-triangle PRIVATE SDL3::SDL3 Vulkan::Vulkan)

