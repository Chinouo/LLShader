#ifndef MODEL_INSTANCE_HPP
#define MODEL_INSTANCE_HPP

#include <vulkan/vulkan.h>

#include <memory>

#include "3rd/imgui/imgui.h"
#include "demos/common/game_object.hpp"
#include "demos/common/model_prototype.hpp"
#include "demos/common/rotate_mixin.hpp"

namespace LLShader {

class ModelInstance : public GameObject, public QuaternionRotateMixin {
 public:
  ModelInstance(std::shared_ptr<ModelProtoType>& proto) : proto_type(proto) {}

  // clone
  ModelInstance(const ModelInstance& rhs)
      : proto_type(rhs.proto_type), world_position(rhs.world_position) {}

  inline void setWorldPosition(glm::vec3& new_position) {
    world_position = new_position;
  }

  inline glm::mat4 getModelMatrix() const {
    // rotate first, then translate.
    return glm::translate(mat4_identity * glm::mat4_cast(rotation),
                          world_position);
  }

  inline glm::vec3 getWorldPosition() const { return world_position; }

  glm::vec3 world_position{0.f, 0.f, 0.f};

 private:
  const std::shared_ptr<ModelProtoType> proto_type;
};

/// currently i have no idea on how to writer delegent draw.
/// some func for bind and draw.
// class DrawableModelInstancesManager : public ModelInstance {
//  public:
//   DrawableModelInstance(const ModelInstance& rhs) : ModelInstance(rhs) {}

//   inline void bindVertexBuffer(VkCommandBuffer commandbuffer) {
//     vkCmdBindVertexBuffers(commandbuffer, 0, 1, );
//     vkCmdBindIndexBuffer();
//     vkCmdDrawIndexed()
//   }
// };

}  // namespace LLShader

#endif