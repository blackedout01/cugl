cmake_minimum_required(VERSION 3.26)

project(CuGL)

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CACHE{CUGL_VULKAN_SDK_ROOT_PATH} STRING "Path to the Vulkan SDK root directoy" "")
set(CACHE{CUGL_VULKAN_SDK_PLATFORM_PATH} STRING "Path to the Vulkan SDK platform directory" "")

if("${CUGL_VULKAN_SDK_ROOT_PATH}" STREQUAL "" AND "${CUGL_VULKAN_SDK_PLATFORM_PATH}" STREQUAL "")
  message(FATAL_ERROR "Vulkan SDK path not configured, please use -DCUGL_VULKAN_SDK_ROOT_PATH=path")
endif()

# NOTE(blackedout): Turn CUGL_VULKAN_SDK_ROOT_PATH into CUGL_VULKAN_SDK_PLATFORM_PATH if the latter is not set
if("${CUGL_VULKAN_SDK_PLATFORM_PATH}" STREQUAL "")
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CUGL_VULKAN_SDK_PLATFORM_PATH ${CUGL_VULKAN_SDK_ROOT_PATH}/macOS)
  else()
    message(FATAL_ERROR "Unconfigured Vulkan SDK root to platform path, please use -DCUGL_VULKAN_SDK_PLATFORM_PATH=path instead")
  endif()
endif()

option(CUGL_BUILD_EXAMPLES "Build the CuGL example programs" ON)

add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)
set(ALLOW_EXTERNAL_SPIRV_TOOLS ON)
add_subdirectory(glslang)

add_library(cugl STATIC include/cugl/cugl.h)
target_link_directories(cugl INTERFACE ${CUGL_VULKAN_SDK_PLATFORM_PATH}/lib)
target_link_libraries(cugl glslang glslang-default-resource-limits)
target_link_libraries(cugl vulkan)
target_sources(cugl
  PRIVATE
    src/core.c
    src/cugl.c
    src/enum_info.c
    src/shader_interface.cpp
    src/util.c
    src/vulkan_helpers.c
    VulkanMemoryAllocator/include/vk_mem_alloc.h
)
set_source_files_properties(VulkanMemoryAllocator/include/vk_mem_alloc.h PROPERTIES LANGUAGE CXX COMPILE_FLAGS -DVMA_IMPLEMENTATION)

target_include_directories(cugl
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CUGL_VULKAN_SDK_PLATFORM_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/glslang
)

if(CUGL_BUILD_EXAMPLES)
  add_subdirectory(glfw)
  add_subdirectory(examples/triangle)
endif()