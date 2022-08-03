#include "pbr_demo.hpp"

#include "engine/matrix.hpp"
#include "util/memory_ext.hpp"

namespace LLShader {

void PBRDemo::init() {
  context = global_matrix_engine.render_manager->getRenderBaseContext();
  global_matrix_engine.input_manager->addListener(this);
  loadModels();
  uploadGPUData();
  setupSets();
  createRenderPass();
  createPipelines();
  createFramebuffers();
}

void PBRDemo::dispose() {}

void PBRDemo::onNotification() {
  auto key_command =
      global_matrix_engine.input_manager->getCurrentCommandState();
  auto cursor_delta =
      global_matrix_engine.input_manager->getCursorPositionDelta();

  auto frame_idx = global_matrix_engine.render_manager->getCurrenFrame();

  auto move_delta = camera.processKeyCommand(key_command);
  camera.move(move_delta);
  camera.rotate(glm::vec3(-cursor_delta.y, cursor_delta.x, 0.f));
  auto p = camera.getPerspectiveProjectionMatrix();
  auto v = camera.getViewMatrix();
  p[1][1] *= -1;
  auto pv = p * v;

  // auto test = glm::mat4(7.7f);

  struct CameraShaderType cam_shader_type {};
  cam_shader_type.ViewProjMatrix = pv;
  cam_shader_type.position = camera.position;
  // cam_shader_type.ViewProjMatrix = test;
  // cam_shader_type.position = glm::vec3(6.6f);

  memcpy((void*)((u_int64_t)camera_uniform.mapped_memory +
                 frame_idx * camera_uniform.dynamic_aligned_size),
         &cam_shader_type, camera_uniform.dynamic_aligned_size);

  // upload each frame
  memcpy(material_uniform.mapped_memory, &material_uniform.material,
         sizeof(material_uniform.material));
}

void PBRDemo::update(double dt) {}

void PBRDemo::drawScene(VkCommandBuffer command_buffer,
                        u_int32_t framebuffer_index) {
  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = scene_resource.pass,
      .framebuffer = scene_resource.framebuffers[framebuffer_index],
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

  VkDeviceSize zero_offset[] = {0};
  u_int32_t dynamic_offset[] = {camera_uniform.dynamic_aligned_size *
                                framebuffer_index};

  vkCmdBeginRenderPass(command_buffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    demo_pipeline.pbr_pipeline);

  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          demo_pipeline.pbr_pipeline_layout, 0, 1,
                          &sets_info.global_data_set, 1, dynamic_offset);

  vkCmdBindVertexBuffers(command_buffer, 0, 1, &sphere_data.vert_buffer,
                         zero_offset);

  vkCmdBindIndexBuffer(command_buffer, sphere_data.idx_buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  // bad code
  vkCmdDrawIndexed(command_buffer,
                   sphere_proto_type->getMeshes().front().indices.size(), 1, 0,
                   0, 0);

  vkCmdEndRenderPass(command_buffer);
}

void PBRDemo::drawUI() {
  sphere_instance->drawUIComponent();

  bool show_camera_option = true;
  {
    ImGui::Begin("camera", &show_camera_option, ImGuiWindowFlags_MenuBar);
    auto pos = camera.getPosition();

    ImGui::Text("x: %.2f y: %.2f z: %.2f", pos.x, pos.y, pos.z);

    auto up = camera.getUp();
    auto right = camera.getRight();
    auto front = camera.getForward();

    ImGui::Text("up: %.2f, %.2f, %.2f", up.x, up.y, up.z);
    ImGui::Text("right: %.2f, %.2f, %.2f", right.x, right.y, right.z);
    ImGui::Text("front: %.2f, %.2f, %.2f", front.x, front.y, front.z);
    ImGui::End();
  }

  ImGui::Begin("DemoOption", &show_demo_option, ImGuiWindowFlags_MenuBar);
  // material toolkit
  {
    ImGui::DragFloat("metalic", &material_uniform.material.metalic, 0.01f, 0.f,
                     1.f, "%.2f ");
    ImGui::DragFloat("roughness", &material_uniform.material.roughness, 0.01f,
                     0.f, 1.f, "%.2f ");
  }
  ImGui::End();
}

void PBRDemo::loadModels() {
  sphere_proto_type =
      std::make_shared<ModelProtoType>("./demos/pbr/assets/sphere/sphere.obj");
  sphere_instance = std::make_shared<ModelInstance>(sphere_proto_type);
}

void PBRDemo::uploadGPUData() {
  auto& render_manager = global_matrix_engine.render_manager;
  const auto& meshes = sphere_proto_type->getMeshes();

  // vert & idx buffer
  {
    // note: actually, my model only have one mesh, for loop is useless.
    for (auto& mesh : meshes) {
      render_manager->createDeviceOnlyBuffer(
          sphere_data.vert_buffer, sphere_data.vert_memory,
          mesh.getVerticesByteSize(), (void*)(mesh.vertices.data()),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      render_manager->createDeviceOnlyBuffer(
          sphere_data.idx_buffer, sphere_data.idx_memory,
          mesh.getIndicesByteSize(), (void*)(mesh.indices.data()),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
  }

  // uniform buffer
  {
    // camera uniform (dynamic)
    {
      render_manager->createBufferAndBindMemory(
          camera_uniform.buffer, camera_uniform.memory,
          getDynamicAlignedSize2(sizeof(CameraShaderType)) *
              MAX_FRAMES_IN_FLIGHT,
          nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

      // alloc aligned memory
      auto sz = getDynamicAlignedSize2(sizeof(CameraShaderType));
      camera_uniform.dynamic_aligned_size = sz;
      camera_uniform.shader_type =
          (CameraShaderType*)aligned_malloc(MAX_FRAMES_IN_FLIGHT * sz, sz);

      // auto r1 = getDynamicAlignedSize2(0);    // 1
      // auto r2 = getDynamicAlignedSize2(1);    // 1
      // auto r3 = getDynamicAlignedSize2(2);    // 2
      // auto r4 = getDynamicAlignedSize2(3);    // 4
      // auto r5 = getDynamicAlignedSize2(16);   // 16
      // auto r6 = getDynamicAlignedSize2(80);   // 128
      // auto r7 = getDynamicAlignedSize2(96);   // 128
      // auto r8 = getDynamicAlignedSize2(129);  // 256

      // auto x = 96;
      // void* test = (void*)aligned_malloc(MAX_FRAMES_IN_FLIGHT * x, x);
      // assert(test);

      vkMapMemory(context.device, camera_uniform.memory, 0,
                  MAX_FRAMES_IN_FLIGHT * sz, 0, &camera_uniform.mapped_memory);

      for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        CameraShaderType* data =
            (CameraShaderType*)((u_int64_t)(camera_uniform.shader_type) +
                                sz * i);

        // (*data).ViewProjMatrix =
        //     camera.getPerspectiveProjectionMatrix() * camera.getViewMatrix();
        // (*data).ViewProjMatrix[1][1] *= -1;
        // (*data).position = camera.position;

        (*data).ViewProjMatrix = glm::mat4(7.7f);
        (*data).position = glm::vec3(6.6f);

        VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        range.memory = camera_uniform.memory;
        range.size = camera_uniform.dynamic_aligned_size;
        range.offset = (VkDeviceSize)(i * sz);
        range.pNext = nullptr;
        memcpy((void*)((u_int64_t)camera_uniform.mapped_memory + range.offset),
               data, range.size);
        vkFlushMappedMemoryRanges(context.device, 1, &range);
      }
    }

    // point light uniform(non-dynamic)
    {
      point_light_uniform.shader_type.color = glm::vec3(1.f, 1.f, 1.f);
      point_light_uniform.shader_type.position = glm::vec3(20.f, 20.f, 20.f);
      render_manager->createBufferAndBindMemory(
          point_light_uniform.buffer, point_light_uniform.memory,
          sizeof(PointLightShaderType), &point_light_uniform.shader_type,
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    // material uniform (non-dynamic)
    {
      material_uniform.material.metalic = 0.3f;
      material_uniform.material.roughness = 1.0f;
      render_manager->createBufferAndBindMemory(
          material_uniform.buffer, material_uniform.memory,
          sizeof(MaterialShaderType), &material_uniform.material,
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      vkMapMemory(context.device, material_uniform.memory, 0,
                  sizeof(MaterialShaderType), 0,
                  &material_uniform.mapped_memory);
    }
  }
}

void PBRDemo::setupSets() {
  // layouts
  {
    // global data
    VkDescriptorSetLayoutBinding camera_dynamic_binding{};

    // camera uniform
    camera_dynamic_binding.binding = 0;
    camera_dynamic_binding.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    camera_dynamic_binding.descriptorCount = 1;
    camera_dynamic_binding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    camera_dynamic_binding.pImmutableSamplers = nullptr;

    // point light uniform
    VkDescriptorSetLayoutBinding point_light_binding{};
    point_light_binding.binding = 1;
    point_light_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    point_light_binding.descriptorCount = 1;
    point_light_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    point_light_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding material_binding{};
    material_binding.binding = 2;
    material_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    material_binding.descriptorCount = 1;
    material_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    material_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = {
        camera_dynamic_binding, point_light_binding, material_binding};

    VkDescriptorSetLayoutCreateInfo global_data_layout_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    global_data_layout_info.bindingCount = 3;
    global_data_layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(
            context.device, &global_data_layout_info, nullptr,
            &sets_info.global_data_set_layout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor set layout!");
    }
  }

  // alloc set
  {
    std::array<VkDescriptorSetLayout, 1> layouts = {
        sets_info.global_data_set_layout,
    };
    std::array<VkDescriptorSet, 1> sets = {
        sets_info.global_data_set,
    };

    VkDescriptorSetAllocateInfo set_alloc_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};

    set_alloc_info.descriptorPool = context.descriptorPool;
    set_alloc_info.descriptorSetCount = sets.size();
    set_alloc_info.pSetLayouts = layouts.data();
    set_alloc_info.pNext = nullptr;

    if (vkAllocateDescriptorSets(context.device, &set_alloc_info,
                                 sets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // copy back, because we make a temp variable for layouts and sets.
    sets_info.global_data_set_layout = layouts[0];
    sets_info.global_data_set = sets[0];
  }

  // update set data
  {
    // global data
    // camera uniform
    VkDescriptorBufferInfo camera_uniform_buf_info{};
    camera_uniform_buf_info.buffer = camera_uniform.buffer;
    camera_uniform_buf_info.offset = 0;
    camera_uniform_buf_info.range = camera_uniform.dynamic_aligned_size;

    VkWriteDescriptorSet camera_uniform_writer{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    camera_uniform_writer.dstSet = sets_info.global_data_set;
    camera_uniform_writer.dstBinding = 0;
    camera_uniform_writer.descriptorCount = 1;
    camera_uniform_writer.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    camera_uniform_writer.dstArrayElement = 0;
    camera_uniform_writer.pBufferInfo = &camera_uniform_buf_info;

    // point light uniform
    VkDescriptorBufferInfo point_light_uniform_buf_info{};
    point_light_uniform_buf_info.buffer = point_light_uniform.buffer;
    point_light_uniform_buf_info.offset = 0;
    point_light_uniform_buf_info.range = sizeof(PointLightShaderType);

    VkWriteDescriptorSet point_light_uniform_writer{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    point_light_uniform_writer.dstSet = sets_info.global_data_set;
    point_light_uniform_writer.dstBinding = 1;
    point_light_uniform_writer.descriptorCount = 1;
    point_light_uniform_writer.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    point_light_uniform_writer.dstArrayElement = 0;
    point_light_uniform_writer.pBufferInfo = &point_light_uniform_buf_info;

    // material uniform
    VkDescriptorBufferInfo material_uniform_buf_info{};
    material_uniform_buf_info.buffer = material_uniform.buffer;
    material_uniform_buf_info.offset = 0;
    material_uniform_buf_info.range = sizeof(MaterialShaderType);

    VkWriteDescriptorSet material_uniform_writer{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    material_uniform_writer.dstSet = sets_info.global_data_set;
    material_uniform_writer.dstBinding = 2;
    material_uniform_writer.descriptorCount = 1;
    material_uniform_writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    material_uniform_writer.dstArrayElement = 0;
    material_uniform_writer.pBufferInfo = &material_uniform_buf_info;

    std::array<VkWriteDescriptorSet, 3> writer{
        camera_uniform_writer,
        point_light_uniform_writer,
        material_uniform_writer,
    };

    vkUpdateDescriptorSets(context.device, writer.size(), writer.data(), 0,
                           nullptr);
  }
}

void PBRDemo::createRenderPass() {
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
                         &scene_resource.pass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass!");
  }
}

void PBRDemo::createPipelines() {
  auto render_manager = global_matrix_engine.render_manager;
  // scene shadow
  {
    VkShaderModule vs = render_manager->createShaderMoudule(
        "./demos/pbr/shaders/pbr.vert", shaderc_vertex_shader);

    VkShaderModule fs = render_manager->createShaderMoudule(
        "./demos/pbr/shaders/pbr.frag", shaderc_fragment_shader);
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
    pipeline_layout_c_info.setLayoutCount = 1;

    // all set has same layout, so we use index 0.
    VkDescriptorSetLayout set_layouts[] = {
        sets_info.global_data_set_layout,
    };

    pipeline_layout_c_info.pSetLayouts = set_layouts;
    if (vkCreatePipelineLayout(context.device, &pipeline_layout_c_info, nullptr,
                               &demo_pipeline.pbr_pipeline_layout) !=
        VK_SUCCESS) {
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
        .layout = demo_pipeline.pbr_pipeline_layout,
        .renderPass = scene_resource.pass,
    };

    if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &demo_pipeline.pbr_pipeline) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(context.device, vs, nullptr);
    vkDestroyShaderModule(context.device, fs, nullptr);
  }
}

void PBRDemo::createFramebuffers() {
  auto& render_manager = global_matrix_engine.render_manager;

  auto& swapchain_image_views = render_manager->getSwapchainImageViews();

  auto extent = context.extent;
  auto device = context.device;
  scene_resource.framebuffers.resize(swapchain_image_views.size());
  scene_resource.depth_resources.resize(swapchain_image_views.size());
  // depth resources
  {
    for (size_t i = 0; i < scene_resource.depth_resources.size(); ++i) {
      render_manager->createDepthResource(
          scene_resource.depth_resources[i].depth_image, context.extent,
          VK_FORMAT_D32_SFLOAT,
          scene_resource.depth_resources[i].depth_image_view,
          scene_resource.depth_resources[i].depth_memory);
    }
  }

  for (size_t i = 0; i < scene_resource.framebuffers.size(); ++i) {
    VkImageView attachments[] = {
        swapchain_image_views[i],
        scene_resource.depth_resources[i].depth_image_view,
    };

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = scene_resource.pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &scene_resource.framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

}  // namespace LLShader