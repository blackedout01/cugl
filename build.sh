if [ "$1" = "conf" ]; then
    # NOTE(blackedout): CMAKE_EXPORT_COMPILE_COMMANDS is for clangd (vscode extension)
    cmake -B out -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCUGL_VULKAN_SDK_ROOT_PATH=/Volumes/INTENSO/cubyz/glfw-vk-template/vulkan/vulkansdk-macos-1.4.328.1
fi

cmake --build out --parallel 4