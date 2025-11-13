cmake_minimum_required(VERSION 3.26)

project(CuGL)

# -DCMAKE_EXPORT_COMPILE_COMMANDS=1
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(cugl)

target_sources(cugl
  PRIVATE
    src/cugl.c
    src/cugl_core.c
    src/shader_interface.cpp

  PUBLIC
    FILE_SET myHeaders
    TYPE HEADERS
    BASE_DIRS
      include
    FILES
      include/cugl/cugl.h
)

target_include_directories(cugl
  PRIVATE
    ../glfw-vk-template/vulkan/vulkansdk-macos-1.4.328.1/macOS/include
    VulkanMemoryAllocator/include
    glslang
)