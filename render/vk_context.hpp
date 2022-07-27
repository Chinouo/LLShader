#ifndef VK_CONTEXT_HPP
#define VK_CONTEXT_HPP

#include <optional>
#include <set>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <vulkan/vulkan.hpp>

#include "3rd/glfw/include/GLFW/glfw3.h"

namespace LLShader {

typedef struct {
  VkInstance instance;
  VkPhysicalDevice physical_device;
} VkContext;

inline const VkApplicationInfo app_info{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
    .pEngineName = "LLShader",
    .engineVersion = VK_MAKE_VERSION(0, 0, 1),
    .apiVersion = VK_API_VERSION_1_3,
};
/// TODO: 去重
/// 不用 const 因为 debug 模式 可能需要填充一些额外的 layer
inline std::vector<const char *> instance_extensions{
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};

// 创建 instance 和 device 共用的
inline std::vector<const char *> layers{

};

inline std::vector<const char *> device_extensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",
};

class VkHolder final {
 public:
  friend class Matrix;

  VkHolder() = default;
  VkHolder(const VkHolder &) = delete;
  ~VkHolder();

  VkContext getVkContext() const;

 private:
  void init();

  // this func only called by engine shut down.
  void dispose();

  void createVkInstance();
  void pickVkPhysicalDevice();

  VkInstance instance_;
  VkPhysicalDevice physical_device_;

#ifdef DEBUG
  VkDebugUtilsMessengerEXT debugMessenger;
#endif
};
}  // namespace LLShader

#endif