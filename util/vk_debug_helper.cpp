#ifdef DEBUG

#include "vk_debug_helper.hpp"

namespace LLShader {

VkDebugUtilsMessengerEXT DebugCreateMessenger(VkInstance instance) {
  VkDebugUtilsMessengerEXT messenger;

  // set up messenger
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = DebugCallback,
      .pUserData = nullptr,
  };
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");

  assert(func != nullptr);
  VkResult debugCreateResult =
      func(instance, &debugCreateInfo, nullptr, &messenger);
  assert(debugCreateResult == VK_SUCCESS);

  return messenger;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
  }
  return VK_FALSE;
}

void DebugDestoryMessenger(const VkInstance instance,
                           VkDebugUtilsMessengerEXT messenger) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  assert(func != nullptr);
  func(instance, messenger, nullptr);
}

void DebugPopulateVailidationLayerInfo(std::vector<const char*>& layers) {
  layers.push_back("VK_LAYER_KHRONOS_validation");
}

void DebugPopulateInstanceExt(std::vector<const char*>& ext) {
  ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

// 检查是否支持 Validation Layer
bool CheckValidationLayerSupport(
    const std::vector<VkLayerProperties>& availableLayer) {
  std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
  for (const char* layerName : validationLayers) {
    for (const auto& layerProperty : availableLayer) {
      if (strcmp(layerProperty.layerName, layerName) == 0) {
        LogUtil::LogI("Support 'VK_LAYER_KHRONOS_validation'.");
        return true;
      };
    }
  }
  return false;
}

}  // namespace LLShader

#endif
