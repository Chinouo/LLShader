#include "vk_context.hpp"

#ifdef DEBUG
#include "util/vk_debug_helper.hpp"
#endif

#include "render/render_manager.hpp"

#include "log/log.hpp"

namespace LLShader{

    VkContext VkHolder::getVkContext() const{
        return { instance_,
                 physical_device_};
    }

    VkHolder::~VkHolder(){

    }


    void VkHolder::init(){
        createVkInstance();
        #ifdef DEBUG
        debugMessenger = DebugCreateMessenger(instance_);
        #endif
        pickVkPhysicalDevice();
    }

    void VkHolder::dispose(){
        #ifdef DEBUG
        DebugDestoryMessenger(instance_, debugMessenger);
        #endif
        if(instance_ != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
    }

    void VkHolder::createVkInstance(){

        // 添加 glfw 所需要的 扩展
        uint32_t glfwRequiredExtCount;
        const char **glfwRequiredExts = glfwGetRequiredInstanceExtensions(&glfwRequiredExtCount);
        std::vector<const char*> glfwReExt(glfwRequiredExts, glfwRequiredExts + glfwRequiredExtCount);
        instance_extensions.insert(instance_extensions.end(), glfwReExt.begin(), glfwReExt.end());

        #ifdef DEBUG
        DebugPopulateInstanceExt(instance_extensions);
        DebugPopulateVailidationLayerInfo(layers);
        #endif

        VkInstanceCreateInfo instanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            .pApplicationInfo = &app_info,
            .enabledLayerCount =  static_cast<uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
            .ppEnabledExtensionNames = instance_extensions.data(),
        };
            
        if(vkCreateInstance(&instanceCreateInfo, nullptr, &instance_) != VK_SUCCESS){
            throw std::runtime_error("Create Instance Failed.");
        }


    }

    void VkHolder::pickVkPhysicalDevice(){
        uint32_t device_count;
        vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
        std::vector<VkPhysicalDevice> physical_devices(device_count);
        if(device_count == 0) throw std::runtime_error("No physicalDevice detected.\n");
        vkEnumeratePhysicalDevices(instance_, &device_count, physical_devices.data());

        VkPhysicalDeviceProperties deviceProperties;
        
        LogUtil::LogI(std::string("We always pick up first detected GPU.\n"));
        LogUtil::LogI(std::string("If you prefer another one, modify `ChooseSinglePhysicalDevice()` at vk_creater.cpp\n"));
        LogUtil::LogI(std::string("Here list all detected device name:\n"));

        for(const auto& gpu : physical_devices){
            vkGetPhysicalDeviceProperties(gpu, &deviceProperties);
            LogUtil::LogI(std::string(deviceProperties.deviceName) + '\n');
        }
        physical_device_ = physical_devices.front();
    }

    // void VkHolder::findRequiredQueueFamily(){

    //     uint32_t queue_family_count;
    //     vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);
    //     std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
    //     vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_family_properties.data());

    //     for(uint32_t i = 0; i < queue_family_properties.size(); ++i){
    //         if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
    //             q_family_indices_.graphic_family = i;
    //             VkBool32 presentSupport = false;
    //             vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, i, surface_, &presentSupport);
    //             if(presentSupport){
    //                 q_family_indices_.present_family = i;
    //                 break;
    //             }
    //         }
    //     }

    //     if(!q_family_indices_.isComplete()) throw std::runtime_error("Queue Family we need do not supported.");
    // };           

    // void VkHolder::createVkDevice(){
    //     // TODO: make it populate.
    //     VkPhysicalDeviceFeatures deviceFeature{
    //         .samplerAnisotropy = VK_TRUE,
    //     };
        
    //     // set up queue
    //     // currently, we only want graphic and present family.
    //     std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    //     std::set<uint32_t> uniqueQueueIndices{
    //         q_family_indices_.graphic_family.value(),
    //         q_family_indices_.present_family.value(),
    //     };
    //     const float queuePriority = 1.0f;
    //     for(uint32_t queueFamily : uniqueQueueIndices){
    //         VkDeviceQueueCreateInfo info{
    //             .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    //             .queueFamilyIndex = queueFamily,
    //             .queueCount = 1,
    //             .pQueuePriorities = &queuePriority,
    //         };
    //         queueCreateInfos.push_back(info);
    //     }

    //     VkDeviceCreateInfo deviceCreateInfo{
    //         .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    //         .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
    //         .pQueueCreateInfos = queueCreateInfos.data(),
    //         .enabledLayerCount = static_cast<uint32_t>(layers.size()),
    //         .ppEnabledLayerNames = layers.data(),
    //         .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
    //         .ppEnabledExtensionNames = device_extensions.data(),
    //         .pEnabledFeatures = &deviceFeature,
    //     };

    //     if(vkCreateDevice(physical_device_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS){
    //         throw std::runtime_error("Create VkDevice failed.");
    //     }
    // }


}