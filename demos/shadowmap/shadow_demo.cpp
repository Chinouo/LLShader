#include "shadow_demo.hpp"

#include "engine/matrix.hpp"
#include "util/assets_helper.hpp"
#include "util/memory_ext.hpp"

namespace LLShader {

void ShadowMapDemo::init() {
  context = global_matrix_engine.render_manager->getRenderBaseContext();
  global_matrix_engine.input_manager->addListener(this);
  loadVertices();
  loadTextures();
  createGPUDatas();
  setupSetAndLayout();
  createRenderPasses();
  createPipelines();
  createFrameBuffers();
}

void ShadowMapDemo::dispose() {
  auto device = context.device;
  aligned_mfree(camera_data.obj.data);

  vkDestroyDescriptorSetLayout(device, set_config.global_data_set_layout,
                               nullptr);
}

void ShadowMapDemo::loadVertices() {
  loadObjToMesh("./demos/shadowmap/assets/mary/Marry.obj", mary.mesh);
  loadObjToMesh("./demos/shadowmap/assets/floor/floor.obj", floor.mesh);
  loadObjToMesh("./demos/shadowmap/assets/cube/cube.obj", lamp.mesh);
}

void ShadowMapDemo::loadTextures() {
  mary.texture = global_matrix_engine.render_manager->loadTexture2D(
      "./demos/shadowmap/assets/mary/MC003_Kozakura_Mari.png");
}

void ShadowMapDemo::createGPUDatas() {
  auto& render_manager = global_matrix_engine.render_manager;
  auto& swapchain_image_views =
      global_matrix_engine.render_manager->getSwapchainImageViews();
  auto sz = swapchain_image_views.size();

  // mary
  {
    render_manager->createDeviceOnlyBuffer(
        mary.vert_buffer, mary.vert_memory,
        mary.mesh.vertices.size() * sizeof(VertexData),
        mary.mesh.vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    render_manager->createDeviceOnlyBuffer(
        mary.idx_buffer, mary.idx_memory,
        mary.mesh.indices.size() * sizeof(u_int32_t), mary.mesh.indices.data(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  // floor
  {
    render_manager->createDeviceOnlyBuffer(
        floor.vert_buffer, floor.vert_memory,
        floor.mesh.vertices.size() * sizeof(VertexData),
        floor.mesh.vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    render_manager->createDeviceOnlyBuffer(
        floor.idx_buffer, floor.idx_memory,
        floor.mesh.indices.size() * sizeof(u_int32_t),
        floor.mesh.indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  // cube
  {
    render_manager->createDeviceOnlyBuffer(
        lamp.vert_buffer, lamp.vert_memory,
        lamp.mesh.vertices.size() * sizeof(VertexData),
        lamp.mesh.vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    render_manager->createDeviceOnlyBuffer(
        lamp.idx_buffer, lamp.idx_memory,
        lamp.mesh.indices.size() * sizeof(u_int32_t), mary.mesh.indices.data(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    lamp_instances_data.data.resize(2);  // we have two instance.
    lamp_instances_data.data[0].model = glm::mat4(1.f);
    lamp_instances_data.data[0].color = glm::vec3(0.1, 0.1, 0.1);
    lamp_instances_data.data[1].model = glm::mat4(1.f);
    lamp_instances_data.data[1].color = glm::vec3(0.9, 0.9, 0.9);
    render_manager->createBufferAndBindMemory(
        lamp_instances_data.property_buffer,
        lamp_instances_data.property_memory,
        sizeof(LampInstanceData::InstanceData) *
            lamp_instances_data.data.size(),  // data.data.date, what a mess.
        lamp_instances_data.data.data(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  }

  // dynamic uniform
  {
    // code below is calulating aligned size required.
    size_t min_uniform_aligment =
        global_matrix_engine.vk_holder->getPhysicalDeviceProperties()
            .limits.minUniformBufferOffsetAlignment;
    size_t dynamic_aligment = sizeof(glm::mat4);
    if (min_uniform_aligment > 0) {
      // 这个算法是求 我们需要的大小 和 min_uniform_aligment 的 整数倍数
      // 最接近的那个值
      dynamic_aligment = (dynamic_aligment + min_uniform_aligment - 1) &
                         ~(min_uniform_aligment - 1);
    }

    // we want flush mem by ourselves
    render_manager->createBufferAndBindMemory(
        camera_data.buffer, camera_data.memory,
        dynamic_aligment * MAX_FRAMES_IN_FLIGHT, nullptr,  // we upload later.
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // set up data and upload.
    camera_data.range = dynamic_aligment;
    camera_data.obj.data = (glm::mat4*)aligned_malloc(
        MAX_FRAMES_IN_FLIGHT * sizeof(glm::mat4), dynamic_aligment);

    assert(camera_data.obj.data);

    vkMapMemory(context.device, camera_data.memory, 0,
                MAX_FRAMES_IN_FLIGHT * dynamic_aligment, 0,
                &camera_data.mapped_memory);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      glm::mat4* data = (glm::mat4*)((u_int64_t)(camera_data.obj.data) +
                                     dynamic_aligment * i);
      (*data) =
          camera.getPerspectiveProjectionMatrix() * camera.getViewMatrix();
      //   glm::mat4(1.f, 2.f, 3.f, (float)i / (float)MAX_FRAMES_IN_FLIGHT);
      // no coherent bit set,  so need flush by ourselves.
      VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
      range.memory = camera_data.memory;
      range.size = sizeof(glm::mat4);
      range.offset = (VkDeviceSize)(i * dynamic_aligment);
      range.pNext = nullptr;
      memcpy((void*)((u_int64_t)camera_data.mapped_memory + range.offset), data,
             range.size);
      vkFlushMappedMemoryRanges(context.device, 1, &range);
    }
  }

  // direction light
  {
    direction_light_shadow_pass.depth_resources.resize(sz);

    for (size_t i = 0; i < sz; ++i) {
      render_manager->defaultCreateDepthResource(
          context.extent, direction_light_shadow_pass.depth_resources[i].image,
          direction_light_shadow_pass.depth_resources[i].image_view,
          direction_light_shadow_pass.depth_resources[i].memory);
    }

    auto lookat =
        glm::lookAt(glm::vec3(25.f, 25.f, 25.f), glm::vec3(0.f, 0.f, 0.f),
                    glm::vec3(0.f, 1.f, 0.f));

    auto proj = glm::ortho(-80.f, 80.f, -80.f, 80.f, 0.03f, 500.f);
    proj[1][1] *= -1;
    direction_light.shader_type_data.view_projection_matrix = proj * lookat;
    render_manager->createBufferAndBindMemory(
        direction_light.shader_type_buffer, direction_light.shader_type_mem,
        sizeof(DirectionLightShaderType), &direction_light.shader_type_data,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  }

  // scence
  {
    scence_pass.depth_resources.resize(sz);
    for (size_t i = 0; i < sz; ++i) {
      render_manager->defaultCreateDepthResource(
          context.extent, scence_pass.depth_resources[i].image,
          scence_pass.depth_resources[i].image_view,
          scence_pass.depth_resources[i].memory);
    }
  }

  // sampler
  {
    VkPhysicalDeviceProperties properties =
        global_matrix_engine.vk_holder->getPhysicalDeviceProperties();

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

void ShadowMapDemo::setupSetAndLayout() {
  // layouts
  {
    // global data
    VkDescriptorSetLayoutBinding camera_dynamic_uniform{};

    // camera uniform
    camera_dynamic_uniform.binding = 0;
    camera_dynamic_uniform.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    camera_dynamic_uniform.descriptorCount = 1;
    camera_dynamic_uniform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    camera_dynamic_uniform.pImmutableSamplers = nullptr;

    // dir light uniform
    VkDescriptorSetLayoutBinding dir_light_uniform{};
    dir_light_uniform.binding = 1;
    dir_light_uniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dir_light_uniform.descriptorCount = 1;
    dir_light_uniform.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    dir_light_uniform.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = {camera_dynamic_uniform,
                                               dir_light_uniform};

    VkDescriptorSetLayoutCreateInfo global_data_layout_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    global_data_layout_info.bindingCount = 2;
    global_data_layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(
            context.device, &global_data_layout_info, nullptr,
            &set_config.global_data_set_layout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor set layout!");
    }

    // texture data
    {
      // mary tex
      VkDescriptorSetLayoutBinding texture_binding{};
      texture_binding.binding = 0;  // mary texture
      texture_binding.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      texture_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      texture_binding.descriptorCount = 1;
      texture_binding.pImmutableSamplers = nullptr;

      // dir light shadowmap
      VkDescriptorSetLayoutBinding shadowmap_binding{};
      shadowmap_binding.binding = 1;
      shadowmap_binding.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      shadowmap_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      shadowmap_binding.descriptorCount = 1;
      shadowmap_binding.pImmutableSamplers = nullptr;

      std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
          texture_binding,
          shadowmap_binding,
      };

      VkDescriptorSetLayoutCreateInfo texture_set_layout_info{
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
      texture_set_layout_info.bindingCount = bindings.size();
      texture_set_layout_info.pBindings = bindings.data();

      if (vkCreateDescriptorSetLayout(
              context.device, &texture_set_layout_info, nullptr,
              &set_config.texture_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture set layout!");
      }
    }
  }

  // alloc set
  {
    std::array<VkDescriptorSetLayout, 2> layouts = {
        set_config.global_data_set_layout, set_config.texture_set_layout};
    std::array<VkDescriptorSet, 2> sets = {set_config.global_data_set,
                                           set_config.texture_set};

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
    set_config.global_data_set_layout = layouts[0];
    set_config.global_data_set = sets[0];
    set_config.texture_set_layout = layouts[1];
    set_config.texture_set = sets[1];
  }

  // update set data
  {
    // global data
    // camera uniform
    VkDescriptorBufferInfo camera_uniform_buf_info{};
    camera_uniform_buf_info.buffer = camera_data.buffer;
    camera_uniform_buf_info.offset = 0;
    camera_uniform_buf_info.range = camera_data.range;

    VkWriteDescriptorSet camera_uniform_writer{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    camera_uniform_writer.dstSet = set_config.global_data_set;
    camera_uniform_writer.dstBinding = 0;
    camera_uniform_writer.descriptorCount = 1;
    camera_uniform_writer.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    camera_uniform_writer.dstArrayElement = 0;
    camera_uniform_writer.pBufferInfo = &camera_uniform_buf_info;

    // dir light uniform
    VkDescriptorBufferInfo dir_light_uniform_buf_info{};
    dir_light_uniform_buf_info.buffer = direction_light.shader_type_buffer;
    dir_light_uniform_buf_info.offset = 0;
    dir_light_uniform_buf_info.range = sizeof(direction_light.shader_type_data);

    VkWriteDescriptorSet dir_light_uniform_writer{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    dir_light_uniform_writer.dstSet = set_config.global_data_set;
    dir_light_uniform_writer.dstBinding = 1;
    dir_light_uniform_writer.descriptorCount = 1;
    dir_light_uniform_writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dir_light_uniform_writer.dstArrayElement = 0;
    dir_light_uniform_writer.pBufferInfo = &dir_light_uniform_buf_info;

    // texture data

    // mary tex
    VkDescriptorImageInfo marry_image_info{};
    marry_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    marry_image_info.imageView = mary.texture.texture_image_view;
    marry_image_info.sampler = sampler;

    VkWriteDescriptorSet marry_texture_sampler_writer{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    marry_texture_sampler_writer.dstSet = set_config.texture_set;
    marry_texture_sampler_writer.dstBinding = 0;
    marry_texture_sampler_writer.dstArrayElement = 0;
    marry_texture_sampler_writer.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    marry_texture_sampler_writer.descriptorCount = 1;
    marry_texture_sampler_writer.pImageInfo = &marry_image_info;

    // dir shadowmap tex (NOTE: although we have multi depth, but i do not want
    // write more code here, use index 0 is work but bad!)
    VkDescriptorImageInfo shadowmap_image_info{};
    shadowmap_image_info.imageLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    shadowmap_image_info.imageView =
        direction_light_shadow_pass.depth_resources[0].image_view;
    shadowmap_image_info.sampler = sampler;

    VkWriteDescriptorSet shadowmap_image_writer{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    shadowmap_image_writer.dstSet = set_config.texture_set;
    shadowmap_image_writer.dstBinding = 1;
    shadowmap_image_writer.dstArrayElement = 0;
    shadowmap_image_writer.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowmap_image_writer.descriptorCount = 1;
    shadowmap_image_writer.pImageInfo = &shadowmap_image_info;

    std::array<VkWriteDescriptorSet, 4> writer{
        camera_uniform_writer,
        dir_light_uniform_writer,
        marry_texture_sampler_writer,
        shadowmap_image_writer,
    };

    vkUpdateDescriptorSets(context.device, writer.size(), writer.data(), 0,
                           nullptr);
  }
}

void ShadowMapDemo::createPipelines() {
  auto& render_manager = global_matrix_engine.render_manager;
  auto device = context.device;

  // direction lighy shadowmap pipeline
  {
    VkShaderModule vs = render_manager->createShaderMoudule(
        "./demos/shadowmap/shaders/dir_light_shadowmap.vert",
        shaderc_vertex_shader);

    VkShaderModule fs = render_manager->createShaderMoudule(
        "./demos/shadowmap/shaders/dir_light_shadowmap.frag",
        shaderc_fragment_shader);
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

    std::array<VkVertexInputAttributeDescription, 1> inputs;

    // position
    inputs[0].location = 0;
    inputs[0].binding = 0;
    inputs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    inputs[0].offset = offsetof(VertexData, position);

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

    VkDescriptorSetLayout set_layouts[] = {set_config.global_data_set_layout,
                                           set_config.texture_set_layout};

    pipeline_layout_c_info.pSetLayouts = set_layouts;
    if (vkCreatePipelineLayout(device, &pipeline_layout_c_info, nullptr,
                               &direction_light_shadow_pass.pipeline_layout) !=
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
        .pColorBlendState = nullptr,
        .pDynamicState = nullptr,
        .layout = direction_light_shadow_pass.pipeline_layout,
        .renderPass = direction_light_shadow_pass.pass,
    };

    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
            &direction_light_shadow_pass.pipeline) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
  }

  // scene pipeline
  {
    VkShaderModule vs = render_manager->createShaderMoudule(
        "./demos/shadowmap/shaders/scene.vert", shaderc_vertex_shader);

    VkShaderModule fs = render_manager->createShaderMoudule(
        "./demos/shadowmap/shaders/scene.frag", shaderc_fragment_shader);

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
    VkDescriptorSetLayout set_layouts[] = {set_config.global_data_set_layout,
                                           set_config.texture_set_layout};

    pipeline_layout_c_info.pSetLayouts = set_layouts;
    if (vkCreatePipelineLayout(device, &pipeline_layout_c_info, nullptr,
                               &scence_pass.pipeline_layout) != VK_SUCCESS) {
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
        .layout = scence_pass.pipeline_layout,
        .renderPass = scence_pass.pass,
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr,
                                  &scence_pass.pipeline) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
  }
}

void ShadowMapDemo::createRenderPasses() {
  // direction light pass
  {
    VkAttachmentDescription attach_desp{};
    attach_desp.format = VK_FORMAT_D32_SFLOAT;
    attach_desp.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desp.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attach_desp.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attach_desp.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attach_desp.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attach_desp.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    attach_desp.samples = VK_SAMPLE_COUNT_1_BIT;
    // direction_light_pass_attach_desp.flags =  I dont know

    VkAttachmentReference ref{};
    ref.attachment = 0;
    ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desp{};
    subpass_desp.colorAttachmentCount = 0;
    subpass_desp.pDepthStencilAttachment = &ref;

    // transition layout using dependency
    std::array<VkSubpassDependency, 2> depdencies{};
    depdencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    depdencies[0].dstSubpass = 0;
    depdencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    depdencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    depdencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    depdencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depdencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    depdencies[1].srcSubpass = 0;
    depdencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    depdencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depdencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    depdencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    // change layout to for scence pass to sample
    depdencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    depdencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo create_info{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    create_info.attachmentCount = 1;
    create_info.pAttachments = &attach_desp;
    create_info.dependencyCount = depdencies.size();
    create_info.pDependencies = depdencies.data();
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass_desp;

    if (vkCreateRenderPass(context.device, &create_info, nullptr,
                           &direction_light_shadow_pass.pass)) {
      throw std::runtime_error("Failed to create direction lighy pass!");
    }
  }

  // scence pass
  {
    VkAttachmentDescription color_attach{};
    color_attach.format = context.surface_format.format;
    color_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attach.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attach.samples = VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentReference color_attach_ref{};
    color_attach_ref.attachment = 0;
    color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attach{};
    depth_attach.format = VK_FORMAT_D32_SFLOAT;
    depth_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attach.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    depth_attach.samples = VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentReference depth_attach_ref{};
    depth_attach_ref.attachment = 1;
    depth_attach_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attach_desps{color_attach,
                                                        depth_attach};

    std::array<VkSubpassDependency, 1> depencies{};  // wait swapchain image
    depencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    depencies[0].dstSubpass = 0;
    depencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depencies[0].srcAccessMask = VK_ACCESS_NONE;
    depencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    depencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkSubpassDescription subpass_desp{};
    subpass_desp.colorAttachmentCount = 1;
    subpass_desp.pColorAttachments = &color_attach_ref;
    subpass_desp.pDepthStencilAttachment = &depth_attach_ref;

    VkRenderPassCreateInfo create_info{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    create_info.attachmentCount = attach_desps.size();
    create_info.pAttachments = attach_desps.data();
    create_info.dependencyCount = depencies.size();
    create_info.pDependencies = depencies.data();
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass_desp;
    if (vkCreateRenderPass(context.device, &create_info, nullptr,
                           &scence_pass.pass)) {
      throw std::runtime_error("Failed to create direction lighy pass!");
    }
  }
}

void ShadowMapDemo::createFrameBuffers() {
  auto& swapchain_image_views =
      global_matrix_engine.render_manager->getSwapchainImageViews();
  auto extent = context.extent;
  auto device = context.device;

  auto sz = swapchain_image_views.size();
  // direction light framebuffers
  direction_light_shadow_pass.framebuffers.resize(sz);
  scence_pass.framebuffers.resize(sz);

  for (size_t i = 0; i < sz; ++i) {
    // direction light framebuffers
    {
      VkImageView attachments[] = {
          direction_light_shadow_pass.depth_resources[i].image_view,
      };
      VkFramebufferCreateInfo framebufferInfo{
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = direction_light_shadow_pass.pass,
          .attachmentCount = 1,
          .pAttachments = attachments,
          .width = extent.width,
          .height = extent.height,
          .layers = 1,
      };

      if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                              &direction_light_shadow_pass.framebuffers[i]) !=
          VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
      }
    }

    // scence framebuffers
    {
      VkImageView attachments[] = {
          swapchain_image_views[i],  // color attachment
          scence_pass.depth_resources[i].image_view,
      };
      VkFramebufferCreateInfo framebufferInfo{
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = scence_pass.pass,
          .attachmentCount = 2,
          .pAttachments = attachments,
          .width = extent.width,
          .height = extent.height,
          .layers = 1,
      };

      if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                              &scence_pass.framebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
      }
    }
  }
}

void ShadowMapDemo::onNotification() {
  auto current_frame_idx =
      global_matrix_engine.render_manager->getCurrenFrame();
  // cursor dx 控制 yaw(rotate.y), cursor dy 控制 pitch(rotate.x)
  // 原点在左上角，  y 轴向下，所以鼠标下移是 +
  auto cursor_delta =
      global_matrix_engine.input_manager->getCursorPositionDelta();

  auto move_delta = camera.processKeyCommand(
      global_matrix_engine.input_manager->getCurrentCommandState());

  camera.move(move_delta);
  camera.rotate(glm::vec3(-cursor_delta.y, cursor_delta.x, 0.f));

  auto p = camera.getPerspectiveProjectionMatrix();
  auto v = camera.getViewMatrix();
  p[1][1] *= -1;
  auto pv = p * v;
  auto dy_aligment = 64;
  memcpy((void*)((u_int64_t)camera_data.mapped_memory +
                 current_frame_idx * dy_aligment),
         &pv, dy_aligment);
}

void ShadowMapDemo::update(double dt) {}

void ShadowMapDemo::drawScene(VkCommandBuffer command_buffer,
                              u_int32_t framebuffer_index) {
  const VkDeviceSize zero_offset[] = {0};
  u_int32_t dy_offset = 64;  // hard code, value should be dynamic aligment.
  // dir light shadow pass begin
  {
    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = direction_light_shadow_pass.pass,
        .framebuffer =
            direction_light_shadow_pass.framebuffers[framebuffer_index],
        .renderArea{
            .offset = {0, 0},
            .extent = context.extent,
        },
    };

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].depthStencil = {1.0, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(command_buffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      direction_light_shadow_pass.pipeline);

    VkDescriptorSet sets[] = {set_config.global_data_set,
                              set_config.texture_set};

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            direction_light_shadow_pass.pipeline_layout, 0, 2,
                            sets, 1, &dy_offset);

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &mary.vert_buffer,
                           zero_offset);

    vkCmdBindIndexBuffer(command_buffer, mary.idx_buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffer, mary.mesh.indices.size(), 1, 0, 0, 0);

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &floor.vert_buffer,
                           zero_offset);

    vkCmdBindIndexBuffer(command_buffer, floor.idx_buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffer, floor.mesh.indices.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);
  }

  // scene pass begin
  {
    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = scence_pass.pass,
        .framebuffer = scence_pass.framebuffers[framebuffer_index],
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

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      scence_pass.pipeline);

    VkDescriptorSet sets[] = {set_config.global_data_set,
                              set_config.texture_set};

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            scence_pass.pipeline_layout, 0, 2, sets, 1,
                            &dy_offset);

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &mary.vert_buffer,
                           zero_offset);

    vkCmdBindIndexBuffer(command_buffer, mary.idx_buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffer, mary.mesh.indices.size(), 1, 0, 0, 0);

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &floor.vert_buffer,
                           zero_offset);

    vkCmdBindIndexBuffer(command_buffer, floor.idx_buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffer, floor.mesh.indices.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);
  }
}

void ShadowMapDemo::drawUI() {
  // draw camera position
  {
    bool show_demo_window = true;
    ImGui::Begin("camera", &show_demo_window, ImGuiWindowFlags_MenuBar);
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
}

}  // namespace LLShader
