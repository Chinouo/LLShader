#include "mesh_demo.hpp"

#include "engine/matrix.hpp"
#include "util/assets_helper.hpp"
namespace LLShader {
void MeshDemo::init() {
  context = global_matrix_engine.render_manager->getRenderBaseContext();
  loadModels();
  loadTextures();
  createBuffers();
  setupLayoutAndSets();
  createRenderPass();
  createPipelines();
  createFramebuffers();
  global_matrix_engine.input_manager->addListener(this);  // TODO: mutex
}

void MeshDemo::dispose() {
  VkDevice device = context.device;
  // release texture
  {
    vkDestroyImage(device, mary.texture.texture_image, nullptr);
    vkDestroyImageView(device, mary.texture.texture_image_view, nullptr);
    vkFreeMemory(device, mary.texture.texture_memory, nullptr);
  }
  // release model vert and index

  {
    vkDestroyBuffer(device, mary.model_index_buf, nullptr);
    vkFreeMemory(device, mary.model_index_memory, nullptr);
    vkDestroyBuffer(device, mary.model_vert_buf, nullptr);
    vkFreeMemory(device, mary.model_vert_memory, nullptr);
  }

  // relase depth resources
  {
    for (auto item : depth_resources) {
      vkDestroyImage(device, item.depth_image, nullptr);
      vkDestroyImageView(device, item.depth_image_view, nullptr);
      vkFreeMemory(device, item.depth_memory, nullptr);
    }
  }
}

void MeshDemo::onNotification() {
  auto current_frame_idx =
      global_matrix_engine.render_manager->getCurrenFrame();
  // cursor dx 控制 yaw(rotate.y), cursor dy 控制 pitch(rotate.x)
  // 原点在左上角，  y 轴向下，所以鼠标下移是 +
  auto cursor_delta =
      global_matrix_engine.input_manager->getCursorPositionDelta();

  auto free_delta = camera.processKeyCommand(
      global_matrix_engine.input_manager->getCurrentCommandState());

  camera.move(free_delta);
  camera.rotate(glm::vec3(-cursor_delta.y, cursor_delta.x, 0.f));
  camera_uniform[current_frame_idx].obj.view = camera.getViewMatrix();
  camera_uniform[current_frame_idx].obj.proj =
      camera.getPerspectiveProjectionMatrix();

  // auto fps_pos_delta = fps_camera.processKeyCommand(
  //     global_matrix_engine.input_manager->getCurrentCommandState());
  // fps_camera.move(fps_pos_delta);
  // fps_camera.rotate(glm::vec3(-cursor_delta.y, cursor_delta.x, 0.f));

  // camera_uniform[current_frame_idx].obj.proj =
  //     fps_camera.getPerspectiveProjectionMatrix();
  // camera_uniform[current_frame_idx].obj.view = fps_camera.getViewMatrix();

  camera_uniform[current_frame_idx].obj.proj[1][1] *= -1;

  std::cout << camera_uniform[current_frame_idx].obj.proj[1][1] << std::endl;

  void* data;
  vkMapMemory(context.device, camera_uniform[current_frame_idx].obj_mem, 0,
              sizeof(CameraUniformObject), 0, &data);

  memcpy(data, &camera_uniform[current_frame_idx].obj, sizeof(CameraUniform));
  vkUnmapMemory(context.device, camera_uniform[current_frame_idx].obj_mem);
}

void MeshDemo::update(double dt) {}

void MeshDemo::drawScene(VkCommandBuffer command_buffer,
                         u_int32_t framebuffer_index) {
  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = scene_pass,
      .framebuffer = framebuffers[framebuffer_index],
      .renderArea{
          .offset = {0, 0},
          .extent = context.extent,
      },
  };

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.3f, 0.4f, 0.5f, 1.0f};
  clearValues[1].depthStencil = {1.0, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(command_buffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  VkDeviceSize offset[] = {0};
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    scene_pipeline);

  vkCmdBindVertexBuffers(command_buffer, 0, 1, &mary.model_vert_buf, offset);

  vkCmdBindIndexBuffer(command_buffer, mary.model_index_buf, 0,
                       VK_INDEX_TYPE_UINT32);

  VkDescriptorSet sets[] = {set_infos[framebuffer_index].global_data_set,
                            set_infos[framebuffer_index].texture_set};

  // multi set bind once is ok.
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          scene_pipeline_layout, 0, 2, sets, 0, nullptr);

  vkCmdDrawIndexed(command_buffer, mary.mesh.indices.size(), 1, 0, 0, 0);

  vkCmdEndRenderPass(command_buffer);
}

void MeshDemo::drawUI() {
  // draw camera position
  {
    bool show_demo_window = true;
    ImGui::Begin("camera", &show_demo_window, ImGuiWindowFlags_MenuBar);
    auto pos = fps_camera.getPosition();

    ImGui::Text("x: %.2f y: %.2f z: %.2f", pos.x, pos.y, pos.z);

    auto up = fps_camera.getUp();
    auto right = fps_camera.getRight();
    auto front = fps_camera.getForward();

    ImGui::Text("up: %.2f, %.2f, %.2f", up.x, up.y, up.z);
    ImGui::Text("right: %.2f, %.2f, %.2f", right.x, right.y, right.z);
    ImGui::Text("front: %.2f, %.2f, %.2f", front.x, front.y, front.z);
    ImGui::End();
  }
}

void MeshDemo::loadModels() {
  loadObjToMesh("./demos/obj2mesh/assets/mary/Marry.obj", mary.mesh);
}

void MeshDemo::loadTextures() {
  mary.texture = global_matrix_engine.render_manager->loadTexture2D(
      "./demos/obj2mesh/assets/mary/MC003_Kozakura_Mari.png");
}

void MeshDemo::createBuffers() {
  auto& render_manager = global_matrix_engine.render_manager;
  // model buffer
  {
    render_manager->createDeviceOnlyBuffer(
        mary.model_vert_buf, mary.model_vert_memory,
        mary.mesh.vertices.size() * sizeof(VertexData),
        mary.mesh.vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    render_manager->createDeviceOnlyBuffer(
        mary.model_index_buf, mary.model_index_memory,
        mary.mesh.indices.size() * sizeof(u_int32_t), mary.mesh.indices.data(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  // depth image
  {
    size_t img_cnt = render_manager->getSwapchainImageViews().size();
    depth_resources.resize(img_cnt);
    for (auto& item : depth_resources) {
      render_manager->defaultCreateDepthResource(
          context.extent, item.depth_image, item.depth_image_view,
          item.depth_memory);
    }
  }

  // Set 0 : camera data
  {
    camera_uniform.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      CameraUniformObject& co = camera_uniform[i].obj;

      co.model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f));

      co.proj = camera.getPerspectiveProjectionMatrix();
      co.view = camera.getViewMatrix();
      // co.proj = fps_camera.getPerspectiveProjectionMatrix();
      // co.view = fps_camera.getViewMatrix();
      co.proj[1][1] *= -1;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      render_manager->createBufferAndBindMemory(
          camera_uniform[i].obj_buf, camera_uniform[i].obj_mem,
          sizeof(CameraUniformObject), &camera_uniform[i].obj,  // TODO: init
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    // Set 2 : using combined image
    {
      // create sampler
      VkPhysicalDeviceProperties properties{};
      auto p_device =
          global_matrix_engine.vk_holder->getVkContext().physical_device;
      vkGetPhysicalDeviceProperties(p_device, &properties);

      VkSamplerCreateInfo samplerInfo{};
      samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
      samplerInfo.magFilter = VK_FILTER_LINEAR;
      samplerInfo.minFilter = VK_FILTER_LINEAR;
      samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      samplerInfo.anisotropyEnable = VK_TRUE;
      samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
      samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
      samplerInfo.unnormalizedCoordinates = VK_FALSE;
      samplerInfo.compareEnable = VK_FALSE;
      samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
      samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

      if (vkCreateSampler(context.device, &samplerInfo, nullptr, &sampler) !=
          VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
      }
    }
  }
}

void MeshDemo::setupLayoutAndSets() {
  set_infos.resize(MAX_FRAMES_IN_FLIGHT);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    // layout
    {
      // global
      VkDescriptorSetLayoutBinding ubo_binding{};
      ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      ubo_binding.binding = 0;
      ubo_binding.descriptorCount = 1;
      ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

      VkDescriptorSetLayoutCreateInfo global_set_layout_info{
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
      global_set_layout_info.bindingCount = 1;
      global_set_layout_info.pBindings = &ubo_binding;

      if (vkCreateDescriptorSetLayout(
              context.device, &global_set_layout_info, nullptr,
              &set_infos[i].global_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
      }

      // texture
      VkDescriptorSetLayoutBinding sampler_binding{};
      sampler_binding.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      sampler_binding.binding = 0;
      sampler_binding.descriptorCount = 1;
      sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      sampler_binding.pImmutableSamplers = nullptr;
      VkDescriptorSetLayoutCreateInfo texture_layout_info{
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
      texture_layout_info.bindingCount = 1;
      texture_layout_info.pBindings = &sampler_binding;
      if (vkCreateDescriptorSetLayout(
              context.device, &texture_layout_info, nullptr,
              &set_infos[i].texture_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
      }
      // alloc set
      {
        VkDescriptorSet sets[] = {set_infos[i].global_data_set,
                                  set_infos[i].texture_set};
        VkDescriptorSetLayout set_layouts[] = {set_infos[i].global_set_layout,
                                               set_infos[i].texture_set_layout};
        VkDescriptorSetAllocateInfo desp_set_alloc_info{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        desp_set_alloc_info.descriptorPool = context.descriptorPool;
        desp_set_alloc_info.descriptorSetCount = 2;
        desp_set_alloc_info.pSetLayouts = set_layouts;

        if (vkAllocateDescriptorSets(context.device, &desp_set_alloc_info,
                                     sets) != VK_SUCCESS) {
          throw std::runtime_error("failed to allocate descriptor sets!");
        }

        // alloc not give a reference, so we need copy back.
        set_infos[i].global_data_set = sets[0];
        set_infos[i].texture_set = sets[1];

        assert(set_infos[i].global_data_set);
        assert(set_infos[i].texture_set);
      }
    }
  }
  // update set
  {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      VkDescriptorBufferInfo ubo_buf_info{};
      ubo_buf_info.buffer = camera_uniform[i].obj_buf;
      ubo_buf_info.offset = 0;
      ubo_buf_info.range = sizeof(camera_uniform[i].obj);

      VkWriteDescriptorSet ubo_writer{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
      ubo_writer.dstBinding = 0;
      ubo_writer.descriptorCount = 1;
      ubo_writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      ubo_writer.dstArrayElement = 0;
      ubo_writer.dstSet = set_infos[i].global_data_set;
      ubo_writer.pBufferInfo = &ubo_buf_info;

      VkDescriptorImageInfo marry_image_info{};
      marry_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      marry_image_info.imageView = mary.texture.texture_image_view;
      marry_image_info.sampler = sampler;
      VkWriteDescriptorSet marry_texture_sampler_writer{
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
      marry_texture_sampler_writer.dstSet = set_infos[i].texture_set;
      marry_texture_sampler_writer.dstBinding = 0;
      marry_texture_sampler_writer.dstArrayElement = 0;
      marry_texture_sampler_writer.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      marry_texture_sampler_writer.descriptorCount = 1;
      marry_texture_sampler_writer.pImageInfo = &marry_image_info;

      VkWriteDescriptorSet writers[] = {ubo_writer,
                                        marry_texture_sampler_writer};

      vkUpdateDescriptorSets(context.device, 2, writers, 0, nullptr);
    }
  }
}

void MeshDemo::createRenderPass() {
  VkAttachmentDescription colorAttachment{
      .format = context.surface_format.format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

  };

  VkAttachmentReference colorAttachmentRef{
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentDescription depthAttachment{
      .format = VK_FORMAT_D32_SFLOAT,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
  };

  VkAttachmentReference depthAttachmentRef{
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
      .pDepthStencilAttachment = &depthAttachmentRef,
  };

  std::array<VkSubpassDependency, 1> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment,
                                                        depthAttachment};
  VkRenderPassCreateInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = attachments.size(),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = dependencies.size(),
      .pDependencies = dependencies.data(),
  };

  if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr,
                         &scene_pass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass!");
  }
}

void MeshDemo::createPipelines() {
  auto& render_manager = global_matrix_engine.render_manager;
  auto device = context.device;

  VkShaderModule vs = render_manager->createShaderMoudule(
      "./demos/obj2mesh/shaders/scene.vert", shaderc_vertex_shader);

  VkShaderModule fs = render_manager->createShaderMoudule(
      "./demos/obj2mesh/shaders/scene.frag", shaderc_fragment_shader);

  // note: using this should explicit init member to zero!
  std::array<VkPipelineShaderStageCreateInfo, 2> stages;

  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vs;
  stages[0].pName = "main";
  stages[0].pNext = nullptr;
  stages[0].flags = 0;
  stages[0].pSpecializationInfo = nullptr;

  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fs;
  stages[1].pName = "main";
  stages[1].pNext = nullptr;
  stages[1].flags = 0;
  stages[1].pSpecializationInfo = nullptr;

  std::array<VkVertexInputAttributeDescription, 3> inputs;

  // position
  inputs[0].location = 0;
  inputs[0].binding = 0;
  inputs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  inputs[0].offset = offsetof(VertexData, position);

  // normal
  inputs[1].location = 1;
  inputs[1].binding = 0;
  inputs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  inputs[1].offset = offsetof(VertexData, normal);

  // texcoords
  inputs[2].location = 2;
  inputs[2].binding = 0;
  inputs[2].format = VK_FORMAT_R32G32_SFLOAT;
  inputs[2].offset = offsetof(VertexData, texcoords);

  VkVertexInputBindingDescription binding_desp{
      .binding = 0,
      .stride = sizeof(VertexData),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkPipelineVertexInputStateCreateInfo input_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding_desp,
      .vertexAttributeDescriptionCount = inputs.size(),
      .pVertexAttributeDescriptions = inputs.data(),
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport viewPort{
      .x = 0.f,
      .y = 0.f,
      .width = static_cast<float>(context.extent.width),
      .height = static_cast<float>(context.extent.height),
      .minDepth = 0.f,
      .maxDepth = 1.f,
  };

  VkRect2D scissor{
      .offset = {0, 0},
      .extent = context.extent,
  };

  VkPipelineViewportStateCreateInfo viewport_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewPort,
      .scissorCount = 1,
      .pScissors = &scissor,
  };

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.f,
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachment{
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo colorBlending{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineDepthStencilStateCreateInfo depthStencil{
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineLayoutCreateInfo pipeline_layout_c_info{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipeline_layout_c_info.setLayoutCount = 2;

  // all set has same layout, so we use index 0.
  VkDescriptorSetLayout set_layouts[] = {set_infos[0].global_set_layout,
                                         set_infos[0].texture_set_layout};

  pipeline_layout_c_info.pSetLayouts = set_layouts;
  if (vkCreatePipelineLayout(device, &pipeline_layout_c_info, nullptr,
                             &scene_pipeline_layout) != VK_SUCCESS) {
    throw std::runtime_error("Create pipelineLayout failed.");
  };

  VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = stages.size(),
      .pStages = stages.data(),
      .pVertexInputState = &input_state,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = nullptr,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlending,
      .pDynamicState = nullptr,
      .layout = scene_pipeline_layout,
      .renderPass = scene_pass,
  };

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &scene_pipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, vs, nullptr);
  vkDestroyShaderModule(device, fs, nullptr);
}

void MeshDemo::createFramebuffers() {
  auto& swapchain_image_views =
      global_matrix_engine.render_manager->getSwapchainImageViews();
  auto extent = context.extent;
  auto device = context.device;
  framebuffers.resize(swapchain_image_views.size());
  for (size_t i = 0; i < framebuffers.size(); ++i) {
    VkImageView attachments[] = {
        swapchain_image_views[i],
        depth_resources[i].depth_image_view,
    };

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = scene_pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}
};  // namespace LLShader
