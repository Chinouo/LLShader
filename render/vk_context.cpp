#include "vk_context.hpp"

#ifdef DEBUG
#include "util/vk_debug_helper.hpp"
#endif

#include "log/log.hpp"
#include "render/render_manager.hpp"

namespace LLShader {

VkContext VkHolder::getVkContext() const {
  return {instance_, physical_device_};
}

VkHolder::~VkHolder() {}

void VkHolder::init() {
  createVkInstance();
#ifdef DEBUG
  debugMessenger = DebugCreateMessenger(instance_);
#endif
  pickVkPhysicalDevice();
  getProperties();
}

void VkHolder::dispose() {
#ifdef DEBUG
  DebugDestoryMessenger(instance_, debugMessenger);
#endif
  if (instance_ != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
}

void VkHolder::createVkInstance() {
  // 添加 glfw 所需要的 扩展
  uint32_t glfwRequiredExtCount;
  const char** glfwRequiredExts =
      glfwGetRequiredInstanceExtensions(&glfwRequiredExtCount);
  std::vector<const char*> glfwReExt(glfwRequiredExts,
                                     glfwRequiredExts + glfwRequiredExtCount);
  instance_extensions.insert(instance_extensions.end(), glfwReExt.begin(),
                             glfwReExt.end());

#ifdef DEBUG
  DebugPopulateInstanceExt(instance_extensions);
  DebugPopulateVailidationLayerInfo(layers);
#endif

  VkInstanceCreateInfo instanceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
      .pApplicationInfo = &app_info,
      .enabledLayerCount = static_cast<uint32_t>(layers.size()),
      .ppEnabledLayerNames = layers.data(),
      .enabledExtensionCount =
          static_cast<uint32_t>(instance_extensions.size()),
      .ppEnabledExtensionNames = instance_extensions.data(),
  };

  if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance_) !=
      VK_SUCCESS) {
    throw std::runtime_error("Create Instance Failed.");
  }
}

void VkHolder::pickVkPhysicalDevice() {
  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
  std::vector<VkPhysicalDevice> physical_devices(device_count);
  if (device_count == 0)
    throw std::runtime_error("No physicalDevice detected.\n");
  vkEnumeratePhysicalDevices(instance_, &device_count, physical_devices.data());

  VkPhysicalDeviceProperties deviceProperties;

  LogUtil::LogI(std::string("We always pick up first detected GPU.\n"));
  LogUtil::LogI(
      std::string("If you prefer another one, modify "
                  "`ChooseSinglePhysicalDevice()` at vk_creater.cpp\n"));
  LogUtil::LogI(std::string("Here list all detected device name:\n"));

  for (const auto& gpu : physical_devices) {
    vkGetPhysicalDeviceProperties(gpu, &deviceProperties);
    LogUtil::LogI(std::string(deviceProperties.deviceName) + '\n');
  }
  physical_device_ = physical_devices.front();
}

void VkHolder::getProperties() {
  vkGetPhysicalDeviceProperties(physical_device_, &physical_device_properties_);
  vkGetPhysicalDeviceFeatures(physical_device_, &physical_device_features_);
  vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties_);
  // TODO: implement queue find;
  //   uint32_t queueFamilyCount;
  //   vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
  //   &queueFamilyCount,
  //                                            nullptr);
  //   assert(queueFamilyCount > 0);
  //   queueFamilyProperties.resize(queueFamilyCount);
  //   vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
  //   &queueFamilyCount,
  //                                            queueFamilyProperties.data());
}

}  // namespace LLShader