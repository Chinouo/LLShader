cmake_minimum_required(VERSION 3.0.0)

project(LLShader VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# GLFW
set(GLFW_INCLUDE ${PROJECT_SOURCE_DIR}/3rd/glfw/include)


# Vulkan Dir
set(VULKAN_DIR "/Users/dtup/VulkanSDK/1.3.216.0")
set(vulkan_lib "${VULKAN_DIR}/macOS/lib/libvulkan.1.dylib")
set(VULKAN_INCLUDE "${VULKAN_DIR}/macOS/include")


add_compile_definitions("VK_ICD_FILENAMES=${VULKAN_DIR}/macOS/share/vulkan/icd.d/MoltenVK_icd.json")
add_compile_definitions("VK_LAYER_PATH=${VULKAN_DIR}/macOS/share/vulkan/explicit_layer.d")

add_definitions(-DDEBUG)


add_subdirectory(3rd)
add_subdirectory(demos)

# grab all file 
file(GLOB_RECURSE HEADER_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/demos/*.hpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.hpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/engine/*.hpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/render/*.hpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/log/*.hpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/util/*.hpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/io/*.hpp")

file(GLOB_RECURSE SOURCE_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/demos/*.cpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.cpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/render/*.cpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/log/*.cpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/util/*.cpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/io/*.cpp")

message("${SOURCE_FILES}")
message("${HEADER_FILES}")




# 3rd lib & include
add_library(MATRIX STATIC ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(MATRIX PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(MATRIX PUBLIC stb)
target_link_libraries(MATRIX PUBLIC glfw)
target_link_libraries(MATRIX PUBLIC tinyobjloader)
target_link_libraries(MATRIX PUBLIC imgui-vk)
target_link_libraries(MATRIX PUBLIC glm)
target_link_libraries(MATRIX PUBLIC "${VULKAN_DIR}/macOS/lib/libshaderc_shared.1.dylib") # shaderc dylib

#target_include_directories(MATRIX PUBLIC VULKAN_INCLUDE)
#target_include_directories(MATRIX PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


# entry-point
add_executable(LLShader main.cpp)
target_link_libraries(LLShader PUBLIC MATRIX)
