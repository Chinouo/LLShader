#pragma once

#include "demos/common/camera.hpp"
#include "demos/common/model_instance.hpp"
#include "demos/common/model_prototype.hpp"
#include "render/render_base.hpp"
#include "render/render_manager.hpp"
#include "util/observer.hpp"

namespace LLShader {
class PBRDemo : public RenderBase, public Listener {
  struct SphereGPUData {
    VkBuffer vert_buffer;
    VkDeviceMemory vert_memory;
    VkBuffer idx_buffer;
    VkDeviceMemory idx_memory;
    // dynamic uniform below, contain model data
    VkBuffer instance_buffer;
    VkDeviceMemory instance_memory;
  };

  // gpu uniform

  struct CameraShaderType {
    glm::mat4 ViewProjMatrix;
    glm::vec3 position;
  };

  struct CameraUniform {
    CameraShaderType* shader_type;  // dynamic uniform
    u_int32_t dynamic_aligned_size;
    void* mapped_memory;
    VkBuffer buffer;
    VkDeviceMemory memory;
  } camera_uniform;

  struct PointLightShaderType {
    glm::vec3 position;
    alignas(16) glm::vec3 color;
  };

  struct PointLightUniform {
    PointLightShaderType shader_type;
    VkBuffer buffer;
    VkDeviceMemory memory;
  } point_light_uniform;

  struct MaterialShaderType {
    float roughness;
    float metalic;
  };

  struct MaterialUniform {
    MaterialShaderType material;
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped_memory;
  } material_uniform;

  struct DepthResource {
    VkImage depth_image;
    VkImageView depth_image_view;
    VkDeviceMemory depth_memory;
  };

  struct ScenceResource {
    VkRenderPass pass;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<DepthResource> depth_resources;
  } scene_resource;

  struct DemoSet {
    VkDescriptorSet global_data_set;
    VkDescriptorSetLayout global_data_set_layout;
  } sets_info;

  struct DemoPipelines {
    VkPipeline pbr_pipeline;
    VkPipelineLayout pbr_pipeline_layout;
  } demo_pipeline;

 public:
  PBRDemo() = default;

  void init() override;
  void dispose() override;
  void update(double dt) override;

  void drawScene(VkCommandBuffer command_buffer,
                 u_int32_t framebuffer_index) override;

  void drawUI() override;

  void onNotification() override;

 private:
  RenderBaseContext context;

  void loadModels();
  void uploadGPUData();
  void setupSets();
  void createRenderPass();
  void createPipelines();
  void createFramebuffers();

  std::shared_ptr<ModelProtoType> sphere_proto_type;
  std::shared_ptr<ModelInstance> sphere_instance;

  SphereGPUData sphere_data;

  FPSCamera camera;

  bool show_demo_option{true};
};
}  // namespace LLShader
