#ifndef MESH_DEMO_HPP
#define MESH_DEMO_HPP

#include "demos/common/camera.hpp"
#include "demos/common/shader_type.hpp"
#include "render/render_base.hpp"
#include "render/render_manager.hpp"

namespace LLShader {

/// Demo draw obj with its texture, no material.
class MeshDemo : public RenderBase, public Listener {
 public:
  /// A model only contain 1 texture and 1 mesh, no material.
  typedef struct {
    Mesh mesh;
    Texture2D texture;

    VkBuffer model_vert_buf;
    VkDeviceMemory model_vert_memory;
    VkBuffer model_index_buf;
    VkDeviceMemory model_index_memory;
  } TypicalModel;

  /// depth resource
  typedef struct {
    VkImage depth_image;
    VkImageView depth_image_view;
    VkDeviceMemory depth_memory;
  } DepthResource;

  typedef struct {
    // 4 sets, for global data, buf, tex and sampler.
    VkDescriptorSetLayout global_set_layout;
    VkDescriptorSetLayout texture_set_layout;
    VkDescriptorSet global_data_set;  // set 0
    VkDescriptorSet buf_set;          // set 1 not use
    VkDescriptorSet texture_set;      // set 2
    VkDescriptorSet sampler_set;      // set 3 not use
  } ResourceSetInfo;

  typedef struct {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
  } CameraUniformObject;

  typedef struct {
    CameraUniformObject obj;
    VkBuffer obj_buf;
    VkDeviceMemory obj_mem;
  } CameraUniform;

  void init() override;
  void dispose() override;
  void update(double dt) override;

  void onNotification() override;

  void drawScene(VkCommandBuffer command_buffer,
                 u_int32_t framebuffer_index) override;

  void drawUI() override;

 private:
  void loadModels();
  void loadTextures();
  void createBuffers();
  void setupLayoutAndSets();
  void createRenderPass();
  void createPipelines();
  void createFramebuffers();
  std::vector<DepthResource> depth_resources;
  std::vector<CameraUniform> camera_uniform;
  std::vector<VkFramebuffer> framebuffers;

  // using free camera.
  RenderCamera camera;

  // constrain pitch.
  FPSCamera fps_camera;

  VkPipeline scene_pipeline;
  VkPipelineLayout scene_pipeline_layout;
  VkRenderPass scene_pass;

  RenderBaseContext context;

  std::vector<ResourceSetInfo> set_infos;

  TypicalModel mary;
  TypicalModel floor;
  VkSampler sampler;
};
};  // namespace LLShader

#endif
