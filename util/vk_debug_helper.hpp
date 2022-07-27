#ifdef DEBUG

#ifndef VK_DEBUG_HELPER
#define VK_DEBUG_HELPER

#include <iostream>
#include <vector>
#include <stdexcept>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN 
#endif

#include "3rd/glfw/include/GLFW/glfw3.h"

#include "log/log.hpp"

namespace LLShader{

    VkDebugUtilsMessengerEXT DebugCreateMessenger(VkInstance instance);

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                                 VkDebugUtilsMessageTypeFlagsEXT messageType, 
                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
                                                 void* pUserData);


    void DebugDestoryMessenger(const VkInstance instance, VkDebugUtilsMessengerEXT messenger);

    void DebugPopulateVailidationLayerInfo(std::vector<const char*>& layers);

    void DebugPopulateInstanceExt(std::vector<const char*>& ext);

    // 检查是否支持 Validation Layer
    bool CheckValidationLayerSupport(const std::vector<VkLayerProperties>& availableLayer);

}



#endif

#endif