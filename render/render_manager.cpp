#include "render_manager.hpp"

#include <filesystem>
#include <fstream>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include "3rd/glfw/include/GLFW/glfw3.h"
#include "3rd/imgui/backends/imgui_impl_vulkan.h"
#include "3rd/stb/stb_image.h"
#include "demos/guitest/gui_test.hpp"
#include "demos/obj2mesh/mesh_demo.hpp"
#include "demos/shadow/shadow.hpp"
#include "engine/matrix.hpp"
#include "render/window_manager.hpp"
#include "util/shader_compiler_helper.hpp"

namespace LLShader {

/// util func below
void RenderManager::createBufferAndBindMemory(VkBuffer& buf,
                                              VkDeviceMemory& mem, size_t size,
                                              void* data,
                                              VkBufferUsageFlags usage,
                                              VkMemoryPropertyFlags property) {
  VkBufferCreateInfo buf_c_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // TODO: mark this flag.
  };

  if (vkCreateBuffer(device_, &buf_c_info, nullptr, &buf) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements mem_req;
  vkGetBufferMemoryRequirements(device_, buf, &mem_req);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_req.size,
      .memoryTypeIndex = getMemoryTypeIndex(mem_req.memoryTypeBits, property),
  };

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device_, buf, mem, 0);

  if (data && (property & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
    void* dst_data;
    vkMapMemory(device_, mem, 0, size, 0, &dst_data);
    memcpy(dst_data, data, size);
    vkUnmapMemory(device_, mem);
  }
}

void RenderManager::createDeviceOnlyBuffer(VkBuffer& buf, VkDeviceMemory& mem,
                                           size_t size, void* data,
                                           VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags property) {
  VkBuffer stage_buf;
  VkDeviceMemory stage_mem;
  VkBufferCreateInfo stage_buf_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage =
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  if (vkCreateBuffer(device_, &stage_buf_info, nullptr, &stage_buf) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create stage buffer!");
  }

  VkMemoryRequirements mem_req;
  vkGetBufferMemoryRequirements(device_, stage_buf, &mem_req);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_req.size,
      .memoryTypeIndex = getMemoryTypeIndex(
          mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
  };

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &stage_mem) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate stage memory!");
  }

  vkBindBufferMemory(device_, stage_buf, stage_mem, 0);

  assert(data);

  void* dst_data;
  vkMapMemory(device_, stage_mem, 0, size, 0, &dst_data);
  memcpy(dst_data, data, size);
  vkUnmapMemory(device_, stage_mem);

  // copy data from stage buf -> taget buf
  VkBufferCreateInfo target_buf_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,  // typically, only
                                                          // device local bit.
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  if (vkCreateBuffer(device_, &target_buf_info, nullptr, &buf) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  vkGetBufferMemoryRequirements(device_, buf, &mem_req);

  VkMemoryAllocateInfo target_mem_alloc_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_req.size,
      .memoryTypeIndex = getMemoryTypeIndex(mem_req.memoryTypeBits,
                                            property),  // property device only
  };

  if (vkAllocateMemory(device_, &target_mem_alloc_info, nullptr, &mem) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device_, buf, mem, 0);

  // copy stage buffer to target buffer.
  auto temp_cmdbuf = beginSingleTimeCommands();
  VkBufferCopy copy_region;
  copy_region.dstOffset = 0;
  copy_region.srcOffset = 0;
  copy_region.size = size;
  vkCmdCopyBuffer(temp_cmdbuf, stage_buf, buf, 1, &copy_region);

  endSingleTimeCommands(temp_cmdbuf);

  vkDestroyBuffer(device_, stage_buf, nullptr);
  vkFreeMemory(device_, stage_mem, nullptr);
}

VkCommandBuffer RenderManager::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = g_command_pool_;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void RenderManager::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(g_queue_, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(g_queue_);

  vkFreeCommandBuffers(device_, g_command_pool_, 1, &commandBuffer);
}

void RenderManager::transitionImageLayout(VkImage image, VkFormat format,
                                          VkImageLayout old_layout,
                                          VkImageLayout new_layout) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                               .oldLayout = old_layout,
                               .newLayout = new_layout,
                               .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .image = image,
                               .subresourceRange{
                                   .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                   .baseMipLevel = 0,
                                   .levelCount = 1,
                                   .baseArrayLayer = 0,
                                   .layerCount = 1,
                               }};

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void RenderManager::copyBufferToImage(VkBuffer buffer, VkImage image,
                                      uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(commandBuffer);
}

Texture2D RenderManager::loadTexture2D(const std::string& file) {
  Texture2D texture2D{};
  int width, height, tex_channel;
  stbi_uc* pixels =
      stbi_load(file.c_str(), &width, &height, &tex_channel, STBI_rgb_alpha);

  if (!pixels) throw std::runtime_error("falied to load " + file);

  VkDeviceSize sz = width * height * sizeof(u_int32_t);

  VkBuffer stage_buf;
  VkDeviceMemory stage_mem;
  createBufferAndBindMemory(
      stage_buf, stage_mem, sz, pixels,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  stbi_image_free(pixels);

  createImageAndBindMemory(
      texture2D.texture_image, width, height,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      texture2D.texture_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  transitionImageLayout(texture2D.texture_image, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copyBufferToImage(stage_buf, texture2D.texture_image, width, height);

  transitionImageLayout(texture2D.texture_image, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device_, stage_buf, nullptr);
  vkFreeMemory(device_, stage_mem, nullptr);

  // TODO: support different format, level...
  {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture2D.texture_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &viewInfo, nullptr,
                          &texture2D.texture_image_view) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture image view!");
    }
  }

  return texture2D;
}

void RenderManager::createImageAndBindMemory(VkImage& image, u_int32_t width,
                                             u_int32_t height,
                                             VkImageUsageFlags usage,
                                             VkDeviceMemory& memory,
                                             VkMemoryPropertyFlags property) {
  VkImageCreateInfo image_c_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .extent{
          .width = width,
          .height = height,
          .depth = 1,
      },
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  if (vkCreateImage(device_, &image_c_info, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("create VkImage failed.");
  }

  VkMemoryRequirements mem_req;
  vkGetImageMemoryRequirements(device_, image, &mem_req);
  VkMemoryAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_req.size,
      .memoryTypeIndex = getMemoryTypeIndex(mem_req.memoryTypeBits, property),
  };

  if (vkAllocateMemory(device_, &alloc_info, nullptr, &memory) != VK_SUCCESS) {
    throw std::runtime_error("failed to create marry_tex_mem! ");
  }

  vkBindImageMemory(device_, image, memory, 0);
}

uint32_t RenderManager::getMemoryTypeIndex(uint32_t type_bit,
                                           VkMemoryPropertyFlags property) {
  VkPhysicalDeviceMemoryProperties physcial_device_mem_prop;
  vkGetPhysicalDeviceMemoryProperties(vk_context.physical_device,
                                      &physcial_device_mem_prop);
  for (uint32_t i = 0; i < physcial_device_mem_prop.memoryTypeCount; i++) {
    if ((type_bit & (1 << i)) &&
        (physcial_device_mem_prop.memoryTypes[i].propertyFlags & property) ==
            property) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

VkShaderModule RenderManager::createShaderMoudule(
    const std::string& file, shaderc_shader_kind shaderType) {
  VkShaderModule shaderModule;
  std::vector<uint32_t> spirv;

  std::fstream in(file, std::ios::in);
  if (!in.is_open()) {
    throw std::runtime_error(std::string("Failed to open ") + file);
  }
  in.seekg(0, std::ios::end);  // 从末尾开始计算偏移量
  std::streampos size = in.tellg();
  in.seekg(0, std::ios::beg);  // 从起始位置开始计算偏移量
  std::vector<char> buf(size);
  in.read(buf.data(), size);
  in.close();
  assert(!buf.empty());
  LogUtil::LogD("start assemble " + file + '\n');

  // TODO: using a global persistent compiler.
  shaderc::Compiler c;

  auto result =
      c.CompileGlslToSpv(buf.data(), buf.size(), shaderType, file.c_str());

  if (!result.GetErrorMessage().empty()) {
    LogUtil::LogE(result.GetErrorMessage() + '\n');
  };

  spirv.assign(result.cbegin(), result.cend());

  LogUtil::LogD("assemble done.\n");
  VkShaderModuleCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = spirv.size() * sizeof(uint32_t),
      .pCode = spirv.data(),
  };

  if (vkCreateShaderModule(device_, &info, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("Fail to create shader.");
  };

  return shaderModule;
}

/// member func
void RenderManager::init() {
  vk_context = global_matrix_engine.vk_holder->getVkContext();
  createVkSurface();
  findGraphicAndPresentFamily();
  createVkDevice();
  getRequiredQueues();
  createCommandPool();
  createCommandBuffers();
  createDescriptorPool();
  createSwapchain();
  getSwapchainImages();
  createSwapchainImageViews();
  createSyncObject();
  // @entery-point.
  // TODO: Make demo pickable.
  p_current_draw_context = std::make_unique<MeshDemo>();
  p_current_draw_context->init();
  installIMGUI();
}

void RenderManager::dispose() {
  uninstallIMGUI();
  p_current_draw_context->dispose();

  if (desp_pool_ != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(device_, desp_pool_, nullptr);
  // note: not need to explict free.
  // vkFreeCommandBuffers(device, cmdPool, cmdbuffers_.size(),
  // cmdbuffers_.data());
  if (g_command_pool_ != VK_NULL_HANDLE)
    vkDestroyCommandPool(device_, g_command_pool_, nullptr);

  // dispose swapchain & images & views ...
  {
    for (size_t i = 0; i < swapchain_image_views_.size(); ++i) {
      vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);
      // note: created  & recycle by swapchain, so not need to explict destory.
      // vkDestroyImage(device , swapchain_images_[i], nullptr);
    }
    if (swapchain_ != VK_NULL_HANDLE)
      vkDestroySwapchainKHR(device_, swapchain_, nullptr);
  }

  if (surface_ != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(vk_context.instance, surface_, nullptr);

  // dispose sync obj
  {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (render_finished_semaphores_[i] != VK_NULL_HANDLE)
        vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
      if (image_available_semaphores_[i] != VK_NULL_HANDLE)
        vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
      if (in_flight_fences_[i] != VK_NULL_HANDLE)
        vkDestroyFence(device_, in_flight_fences_[i], nullptr);
    }
  }

  if (device_ != VK_NULL_HANDLE) vkDestroyDevice(device_, nullptr);
}

void RenderManager::defaultCreateDepthResource(
    VkExtent2D extent, VkImage& depth_image, VkImageView& depth_image_view,
    VkDeviceMemory& depth_image_memory) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = extent.width;
  imageInfo.extent.height = extent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_D32_SFLOAT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT;  // TODO: refactor to setable.
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device_, &imageInfo, nullptr, &depth_image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device_, depth_image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = getMemoryTypeIndex(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &depth_image_memory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(device_, depth_image, depth_image_memory, 0);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = depth_image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_D32_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device_, &viewInfo, nullptr, &depth_image_view) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }
}

void RenderManager::beginFrame() {
  VkCommandBuffer cmdBuffer = cmdbuffers_[current_frame];
  vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                        image_available_semaphores_[current_frame],
                        VK_NULL_HANDLE, &current_image_index);

  vkWaitForFences(device_, 1, &in_flight_fences_[current_frame], VK_TRUE,
                  UINT64_MAX);

  vkResetFences(device_, 1, &in_flight_fences_[current_frame]);

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
}

void RenderManager::drawFrame() {
  // note: render not need to explict call IMGUI_NEWFRAME, but need call Draw.
  auto curent_cmdbuf = cmdbuffers_[current_frame];
  p_current_draw_context->drawScene(curent_cmdbuf, current_image_index);

  // draw UI

  // hide or show cursor betweet
  ImGui_ImplVulkan_NewFrame();
  if (!global_matrix_engine.window_manager->isEditMode()) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
  }
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = gui_context.imgui_pass,
      .framebuffer = gui_context.gui_framebuffers_[current_image_index],
      .renderArea{
          .offset = {0, 0},
          .extent = swapchain_extent_,
      },
  };
  vkCmdBeginRenderPass(curent_cmdbuf, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // UI  draw
  p_current_draw_context->drawUI();
  drawGlobalUIToolKit();

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), curent_cmdbuf);
  vkCmdEndRenderPass(curent_cmdbuf);
}

void RenderManager::drawGlobalUIToolKit() {
  bool show_demo_window = true;
  ImGui::ShowDemoWindow(&show_demo_window);
  {
    ImGui::Begin("Switch", &show_demo_window, ImGuiWindowFlags_MenuBar);
    ImGui::Text("ChangeMode");
    if (ImGui::Button("Button")) {
      global_matrix_engine.window_manager->setWindowMode(WindowMode::play);
    }
    ImGui::End();
  }
}

void RenderManager::endFrame() {
  if (vkEndCommandBuffer(cmdbuffers_[current_frame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  VkPipelineStageFlags waitStage[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &image_available_semaphores_[current_frame],
      .pWaitDstStageMask = waitStage,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmdbuffers_[current_frame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &render_finished_semaphores_[current_frame],
  };

  if (vkQueueSubmit(g_queue_, 1, &submitInfo,
                    in_flight_fences_[current_frame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &render_finished_semaphores_[current_frame],
      .swapchainCount = 1,
      .pSwapchains = &swapchain_,
      .pImageIndices = &current_image_index,
  };

  vkQueuePresentKHR(p_queue_, &presentInfo);
  current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderManager::createVkSurface() {
  auto wd = global_matrix_engine.window_manager->window;
  if (glfwCreateWindowSurface(vk_context.instance, wd, nullptr, &surface_) !=
      VK_SUCCESS) {
    throw std::runtime_error("Create WSI failed.\n");
  }
}

void RenderManager::findGraphicAndPresentFamily() {
  uint32_t queue_family_count;
  auto physical_device = vk_context.physical_device;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> queue_family_properties(
      queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           queue_family_properties.data());

  for (uint32_t i = 0; i < queue_family_properties.size(); ++i) {
    if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      family_indices_.graphic_family = i;
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface_,
                                           &presentSupport);
      if (presentSupport) {
        family_indices_.present_family = i;
        break;
      }
    }
  }

  if (!family_indices_.isComplete())
    throw std::runtime_error("Queue Family we need do not supported.");
}

void RenderManager::createVkDevice() {
  // TODO: make it populate.
  VkPhysicalDeviceFeatures deviceFeature{
      .samplerAnisotropy = VK_TRUE,
  };

  // set up queue
  // currently, we only want graphic and present family.
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueIndices{
      family_indices_.graphic_family.value(),
      family_indices_.present_family.value(),
  };
  const float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueIndices) {
    VkDeviceQueueCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    queueCreateInfos.push_back(info);
  }

  VkDeviceCreateInfo deviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = static_cast<uint32_t>(layers.size()),
      .ppEnabledLayerNames = layers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
      .ppEnabledExtensionNames = device_extensions.data(),
      .pEnabledFeatures = &deviceFeature,
  };

  if (vkCreateDevice(vk_context.physical_device, &deviceCreateInfo, nullptr,
                     &device_) != VK_SUCCESS) {
    throw std::runtime_error("Create VkDevice failed.");
  }
}

void RenderManager::getRequiredQueues() {
  vkGetDeviceQueue(device_, family_indices_.graphic_family.value(), 0,
                   &g_queue_);
  vkGetDeviceQueue(device_, family_indices_.present_family.value(), 0,
                   &p_queue_);
}

void RenderManager::createCommandPool() {
  VkCommandPoolCreateInfo pool_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = family_indices_.graphic_family.value(),
  };

  if (vkCreateCommandPool(device_, &pool_info, nullptr, &g_command_pool_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create graphic command pool!");
  }
}

void RenderManager::createCommandBuffers() {
  // init cmdbuffer.
  cmdbuffers_.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = g_command_pool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<u_int32_t>(MAX_FRAMES_IN_FLIGHT),
  };

  if (vkAllocateCommandBuffers(device_, &allocInfo, cmdbuffers_.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void RenderManager::createDescriptorPool() {
  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  auto sz = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * sz;
  pool_info.poolSizeCount = static_cast<uint32_t>(sz);
  pool_info.pPoolSizes = pool_sizes;
  if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &desp_pool_) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptorPool.");
  }
}

void RenderManager::createSwapchain() {
  // 查询swapchian 所需的属性
  SwapchainSupportDetails details;
  auto physical_device = vk_context.physical_device;
  auto surface = surface_;
  auto wd = global_matrix_engine.window_manager->window;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                            &details.surfaceCapabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount,
                                       nullptr);
  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                            &presentModeCount, nullptr);

  if (formatCount == 0 || presentModeCount == 0)
    throw std::runtime_error("Swapchain formats or presentModes is empty.");

  details.surfaceFormats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount,
                                       details.surfaceFormats.data());

  details.presentModes.resize(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, surface, &presentModeCount, details.presentModes.data());

  // 设置swapchain 的 长宽 属性
  auto& capabilities = details.surfaceCapabilities;
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    swapchain_extent_ = capabilities.currentExtent;
  } else {
    // if u using high dpi device, pixel is not size.
    int width, height;
    glfwGetFramebufferSize(wd, &width, &height);
    VkExtent2D actualExtent = {
        .width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                       capabilities.maxImageExtent.width),
        .height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                       capabilities.maxImageExtent.height),
    };
    swapchain_extent_ = actualExtent;
  }

  // 设置 swapchain 的 图像格式
  assert(!details.surfaceFormats.empty());
  for (const auto& surface_format : details.surfaceFormats) {
    if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      swapchain_format_ = surface_format;
    }
  }
  if (swapchain_format_.format == VK_FORMAT_UNDEFINED) {
    swapchain_format_ = details.surfaceFormats.front();
  }

  // 创建 swapchain
  uint32_t image_count = details.surfaceCapabilities.minImageCount + 1;
  if (details.surfaceCapabilities.maxImageCount > 0 &&
      image_count > details.surfaceCapabilities.maxImageCount) {
    image_count = details.surfaceCapabilities.maxImageCount;
  }

  // TODO: make it selectable like game settings.
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

  VkSwapchainCreateInfoKHR swapChainCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = image_count,
      .imageFormat = swapchain_format_.format,
      .imageColorSpace = swapchain_format_.colorSpace,
      .imageExtent = swapchain_extent_,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = details.surfaceCapabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,  // TODO: Opt resize performance
  };

  // if two queues are diff, we need set vkimage sharing mode.
  uint32_t pIndices[] = {
      family_indices_.graphic_family.value(),
      family_indices_.present_family.value(),
  };
  if (pIndices[0] != pIndices[1]) {
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapChainCreateInfo.queueFamilyIndexCount = 2;
    swapChainCreateInfo.pQueueFamilyIndices = pIndices;
  }
  if (vkCreateSwapchainKHR(device_, &swapChainCreateInfo, nullptr,
                           &swapchain_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create swapchain!");
  }
}

void RenderManager::getSwapchainImages() {
  uint32_t swapChainCount;
  vkGetSwapchainImagesKHR(device_, swapchain_, &swapChainCount, nullptr);
  swapchain_images_.resize(swapChainCount);
  vkGetSwapchainImagesKHR(device_, swapchain_, &swapChainCount,
                          swapchain_images_.data());
}

void RenderManager::createSwapchainImageViews() {
  swapchain_image_views_.resize(swapchain_images_.size());
  for (int i = 0; i < swapchain_image_views_.size(); ++i) {
    VkImageViewCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchain_images_[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchain_format_.format,
    };
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    info.subresourceRange.baseMipLevel = 0,
    info.subresourceRange.levelCount = 1,
    info.subresourceRange.baseArrayLayer = 0,
    info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &info, nullptr,
                          &swapchain_image_views_[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image views!");
    }
  }
}

void RenderManager::createSyncObject() {
  // set up sync object.
  image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
  render_finished_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
  in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr,
                          &image_available_semaphores_[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device_, &semaphoreInfo, nullptr,
                          &render_finished_semaphores_[i]) != VK_SUCCESS ||
        vkCreateFence(device_, &fenceInfo, nullptr, &in_flight_fences_[i]) !=
            VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

void RenderManager::installIMGUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  // io.ConfigFlags |= ImGuiConfigFlags_NoMouse,
  ImGui::StyleColorsDark();
  bool result = ImGui_ImplGlfw_InitForVulkan(
      global_matrix_engine.window_manager->window,
      true);  // TODO: set false, handle install by yourself
  assert(result);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = vk_context.instance;
  init_info.PhysicalDevice = vk_context.physical_device;
  init_info.Device = device_;
  init_info.QueueFamily = family_indices_.graphic_family.value();
  init_info.Queue = g_queue_;
  init_info.PipelineCache = nullptr;
  init_info.DescriptorPool = desp_pool_;
  init_info.Subpass = 0;
  init_info.MinImageCount = 2;
  init_info.ImageCount = swapchain_images_.size();
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  // create imgui needed renderpass.
  {
    VkAttachmentDescription colorAttachment{
        .format = swapchain_format_.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // note: demo renderpass
                                                       // should change layout
                                                       // to this!.
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };

    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};

    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr,
                           &gui_context.imgui_pass) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create render pass!");
    }
  }

  result = ImGui_ImplVulkan_Init(&init_info, gui_context.imgui_pass);
  assert(result);
  // upload font
  auto command_pool = g_command_pool_;
  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VkCommandBuffer command_buffer;
  if (vkAllocateCommandBuffers(device_, &allocInfo, &command_buffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  vkResetCommandPool(device_, command_pool, 0);
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(command_buffer, &begin_info);
  ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

  VkSubmitInfo end_info = {};
  end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  end_info.commandBufferCount = 1;
  end_info.pCommandBuffers = &command_buffer;
  vkEndCommandBuffer(command_buffer);
  vkQueueSubmit(g_queue_, 1, &end_info, VK_NULL_HANDLE);
  vkDeviceWaitIdle(device_);
  ImGui_ImplVulkan_DestroyFontUploadObjects();
  vkFreeCommandBuffers(device_, command_pool, 1, &command_buffer);

  // imgui framebuffer.
  gui_context.gui_framebuffers_.resize(swapchain_image_views_.size());
  for (size_t i = 0; i < swapchain_image_views_.size(); ++i) {
    VkImageView attachments[] = {
        swapchain_image_views_[i],
    };

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = gui_context.imgui_pass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = swapchain_extent_.width,
        .height = swapchain_extent_.height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr,
                            &gui_context.gui_framebuffers_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

// TODO: add perform recreate.
void RenderManager::uninstallIMGUI() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  if (!gui_context.imgui_pass)
    vkDestroyRenderPass(device_, gui_context.imgui_pass, nullptr);

  for (size_t i = 0; i < gui_context.gui_framebuffers_.size(); ++i) {
    vkDestroyFramebuffer(device_, gui_context.gui_framebuffers_[i], nullptr);
  }
}
}  // namespace LLShader