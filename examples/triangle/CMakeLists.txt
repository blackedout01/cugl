link_libraries(cugl)
link_libraries(glfw)

add_library(vulkan_paths INTERFACE)
target_compile_definitions(vulkan_paths
  INTERFACE
    -DVULKAN_EXPLICIT_LAYERS_PATH=\"${CUGL_VULKAN_SDK_PLATFORM_PATH}/share/vulkan/explicit_layer.d\"
    -DVULKAN_DRIVER_FILES=\"${CUGL_VULKAN_SDK_PLATFORM_PATH}/share/vulkan/icd.d/MoltenVK_icd.json\"
)

add_executable(triangle main.c)
target_link_libraries(triangle vulkan_paths)