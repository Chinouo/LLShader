#ifndef ROTATION_MIXIN_HPP
#define ROTATION_MIXIN_HPP

#include "3rd/imgui/imgui.h"
#include "util/math_wrap.hpp"

class QuaternionRotateMixin {
 public:
  QuaternionRotateMixin() = default;

  ~QuaternionRotateMixin() = default;

  inline void applyRotation(glm::quat& n_rotation) { rotation *= n_rotation; }

  inline void setRotation(glm::quat& target_rotation) {
    rotation = target_rotation;
  }

  inline void drawUIComponent() {
    auto id = reinterpret_cast<u_int64_t>(this);
    std::string w_lable = "##" + std::to_string(id) + "w";
    std::string x_lable = "##" + std::to_string(id) + "x";
    std::string y_lable = "##" + std::to_string(id) + "y";
    std::string z_lable = "##" + std::to_string(id) + "z";
    ImGui::Begin("Rotation", &show_component_wd, ImGuiWindowFlags_MenuBar);
    ImGui::DragFloat(w_lable.c_str(), &rotation.w, 0.1f, 0.f, 0.f, "z: %.2f ");
    ImGui::DragFloat(x_lable.c_str(), &rotation.x, 0.1f, 0.f, 0.f, "w: %.2f ");
    ImGui::DragFloat(y_lable.c_str(), &rotation.y, 0.1f, 0.f, 0.f, "x: %.2f ");
    ImGui::DragFloat(z_lable.c_str(), &rotation.z, 0.1f, 0.f, 0.f, "y: %.2f ");
    ImGui::End();
  }

  glm::quat rotation{1.f, 0.f, 0.f, 0.f};

 protected:
  bool show_component_wd{true};
};

#endif