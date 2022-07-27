#ifndef RENDER_BASE_HPP
#define RENDER_BASE_HPP

#include <vulkan/vulkan.hpp>

namespace LLShader {

// 每个 Demo 的基类
class RenderBase {
 public:
  friend class RenderManager;

  virtual ~RenderBase() = default;

  virtual void init() = 0;
  virtual void dispose() = 0;
  virtual void update(double dt) = 0;

  // record command
  virtual void drawScene(VkCommandBuffer command_buffer,
                         u_int32_t framebuffer_index) = 0;

  // only need begin and end imgui context, command will be submitted by
  // render_manager class.
  virtual void drawUI() = 0;

  // call should be followed by declare order.
  // virtual void createRenderPass() = 0;
  // virtual void createPipelines() = 0;
  // virtual void createFramebuffers() = 0;

  // virtual VkFramebuffer getCurrentFramebuffer() = 0;
  // virtual void createDesciptorSet() = 0;
};

}  // namespace LLShader

#endif
