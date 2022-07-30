#pragma once

#include "demos/common/camera.hpp"
#include "demos/common/shader_type.hpp"
#include "render/render_base.hpp"
#include "render/render_manager.hpp"

namespace LLShader {

/// Point Light and Cube Visualization currently are not implement.
class ShadowMapDemo : public RenderBase, public Listener {
 public:
  typedef struct {
    VkDescriptorSet global_data_set;
    VkDescriptorSet buffer_set;
    VkDescriptorSet sampler_set;
    VkDescriptorSet texture_set;

    VkDescriptorSetLayout global_data_set_layout;
    VkDescriptorSetLayout buffer_set_layout;    // null at this demo
    VkDescriptorSetLayout sampler__set_layout;  // null at this demo
    VkDescriptorSetLayout texture_set_layout;

  } SetConfig;

  typedef struct {
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
  } DepthResource;

  typedef struct {
    glm::mat4 *data;
  } UniformObj;

  typedef struct {
    UniformObj obj;
    void *mapped_memory;
    size_t range;
    VkBuffer buffer;
    VkDeviceMemory memory;
  } UniformData;

  struct MaryModel {
    Mesh mesh;
    VkBuffer vert_buffer;
    VkDeviceMemory vert_memory;
    VkBuffer idx_buffer;
    VkDeviceMemory idx_memory;
    Texture2D texture;
  } mary;

  struct FloorModel {
    Mesh mesh;
    VkBuffer vert_buffer;
    VkDeviceMemory vert_memory;
    VkBuffer idx_buffer;
    VkDeviceMemory idx_memory;
    Texture2D texture;
  } floor;

  struct LampModel {
    Mesh mesh;
    VkBuffer vert_buffer;
    VkDeviceMemory vert_memory;
    VkBuffer idx_buffer;
    VkDeviceMemory idx_memory;
  } lamp;

  struct LampInstanceData {
    // color and model memory.
    struct InstanceData {
      glm::mat4 model;
      glm::vec3 color;
    };
    std::vector<InstanceData> data;
    VkBuffer property_buffer;
    VkDeviceMemory property_memory;
  } lamp_instances_data;

  struct CameraDataShaderType {
    glm::mat4 view_projection_matrix;
  } camera_shader_data;

  struct DirectionLightShaderType {
    glm::mat4 view_projection_matrix;
  };

  // gen shadow map for dir light
  struct DirectionLightShadowPass {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkRenderPass pass;
    // Pass write depth to framebuffer
    std::vector<DepthResource> depth_resources;
    std::vector<VkFramebuffer> framebuffers;
  } direction_light_shadow_pass;

  // gen shadow map for point light
  struct PointLightShadowPass {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkRenderPass pass;
  } point_light_shadow_pass;

  // draw our scene
  struct ScencePass {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkRenderPass pass;
    // Pass write depth to framebuffer
    // color attachment is come from render_manager.
    std::vector<DepthResource> depth_resources;
    std::vector<VkFramebuffer> framebuffers;
  } scence_pass;

  // draw light for position visualization
  struct LightPass {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkRenderPass pass;
  } light_pass;

  struct PointLight {
    DirectionLightShaderType shader_data;
  } point_light;

  struct DirectionLight {
    glm::vec4 color;
    glm::vec3 position;
    DirectionLightShaderType shader_type_data;
    VkDeviceMemory shader_type_mem;
    VkBuffer shader_type_buffer;
  } direction_light;

  // class member code block
  ShadowMapDemo() = default;
  void init() override;
  void dispose() override;
  void update(double dt) override;

  void drawScene(VkCommandBuffer command_buffer,
                 u_int32_t framebuffer_index) override;

  void drawUI() override;

  void onNotification() override;

 private:
  void loadVertices();
  void loadTextures();
  void createGPUDatas();
  void setupSetAndLayout();
  void createPipelines();
  void createRenderPasses();
  void createFrameBuffers();
  RenderBaseContext context;
  SetConfig set_config;
  // demo variable
  UniformData camera_data;

  FPSCamera camera;

  VkSampler sampler;
};

}  // namespace LLShader