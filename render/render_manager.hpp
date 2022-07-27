#ifndef RENDER_MANAGER_HPP
#define RENDER_MANAGER_HPP

#define MAX_FRAMES_IN_FLIGHT 3

#include <memory>
#include <shaderc/shaderc.hpp>

#include "render/render_base.hpp"
#include "render/vk_context.hpp"

namespace LLShader {

// used for RenderBase to create framebuffers etc..
typedef struct {
  VkDevice device;
  VkSurfaceFormatKHR surface_format;
  VkExtent2D extent;
  VkDescriptorPool descriptorPool;
} RenderBaseContext;

typedef struct {
  VkRenderPass imgui_pass;
  // global imgui framebuffer, use same image with demo, and draw after demo
  // rendered.
  std::vector<VkFramebuffer> gui_framebuffers_;
} GuiContext;

typedef struct {
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  std::vector<VkSurfaceFormatKHR> surfaceFormats;
  std::vector<VkPresentModeKHR> presentModes;

} SwapchainSupportDetails;

struct RenderQueueFamilyIndices {
  std::optional<uint32_t> graphic_family;
  std::optional<uint32_t> present_family;

  bool isComplete() {
    return graphic_family.has_value() && present_family.has_value();
  }
};

typedef struct {
  std::string texture_name;
  VkImage texture_image;
  VkImageView texture_image_view;
  VkDeviceMemory texture_memory;
} Texture2D;

class RenderManager final {
 public:
  friend class Matrix;

  RenderManager() = default;

  RenderManager(const RenderManager&) = delete;

  /// 处理同步 获得swapchain 的image
  void beginFrame();

  /// 调用 demo 的绘制指令
  void drawFrame();

  /// 提交指令
  void endFrame();

  // these two func is used for renderbass to create framebuffers.
  inline const std::vector<VkImageView>& getSwapchainImageViews() const {
    return swapchain_image_views_;
  }
  inline const RenderBaseContext getRenderBaseContext() const {
    return {device_, swapchain_format_, swapchain_extent_, desp_pool_};
  }
  inline uint32_t getCurrenFrame() const { return current_frame; }

  // util funcs for renderbase
  /// this func only used for host visiable flags
  /// it will create buf & mem using given parameter.
  void createBufferAndBindMemory(VkBuffer& buf, VkDeviceMemory& mem,
                                 size_t size, void* data,
                                 VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags property);

  /// using stage buffer to create only device visiable buf.
  void createDeviceOnlyBuffer(VkBuffer& buf, VkDeviceMemory& mem, size_t size,
                              void* data, VkBufferUsageFlags usage,
                              VkMemoryPropertyFlags property);

  /// load simple 2D texture to GPU.
  /// paramater: path to texture file.
  Texture2D loadTexture2D(const std::string& file);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);

  /// it will create buf & mem using given parameter.
  /// used for 2D tex, rgba 32
  void createImageAndBindMemory(VkImage& image, u_int32_t width,
                                u_int32_t height, VkImageUsageFlags usage,
                                VkDeviceMemory& memory,
                                VkMemoryPropertyFlags property);

  VkCommandBuffer beginSingleTimeCommands();

  void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  /// only support load texture image! used by loadTexture2D func
  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout old_layout,
                             VkImageLayout new_layout);

  /// format always D32
  void defaultCreateDepthResource(VkExtent2D extent, VkImage& depth_image,
                                  VkImageView& depth_image_view,
                                  VkDeviceMemory& depth_image_memory);

  uint32_t getMemoryTypeIndex(uint32_t type_bit,
                              VkMemoryPropertyFlags property);

  VkShaderModule createShaderMoudule(const std::string& loc,
                                     shaderc_shader_kind shaderType);

 private:
  void init();
  void dispose();

  // imgui context
  GuiContext gui_context{VK_NULL_HANDLE};
  void drawGlobalUIToolKit();
  void installIMGUI();
  void uninstallIMGUI();

  // current draw context
  std::unique_ptr<RenderBase> p_current_draw_context;

  // used for init()
  void createSwapchain();
  void getSwapchainImages();
  void createSwapchainImageViews();
  void createSyncObject();
  void createCommandBuffers();

  // call follow the sequence.
  void createVkSurface();
  void findGraphicAndPresentFamily();
  void createVkDevice();
  void getRequiredQueues();
  void createCommandPool();
  void createDescriptorPool();

  // vulkan context
  VkContext vk_context;

  // WSI
  VkSurfaceKHR surface_;

  // manage resource
  VkDevice device_;
  VkDescriptorPool desp_pool_;
  VkCommandPool g_command_pool_;

  // sync obj
  std::vector<VkSemaphore> image_available_semaphores_;
  std::vector<VkSemaphore> render_finished_semaphores_;
  std::vector<VkFence> in_flight_fences_;
  std::vector<VkCommandBuffer> cmdbuffers_;
  uint32_t current_frame{0};
  uint32_t current_image_index{0};

  // render queue
  RenderQueueFamilyIndices family_indices_;
  VkQueue g_queue_;  // graphic
  VkQueue p_queue_;  // present

  // swapchain
  VkSwapchainKHR swapchain_;
  VkExtent2D swapchain_extent_;
  VkSurfaceFormatKHR swapchain_format_;
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_image_views_;
};

}  // namespace LLShader

#endif