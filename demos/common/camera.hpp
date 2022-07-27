#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <cmath>
#include <iostream>

#include "io/input_manager.hpp"
#include "util/math_wrap.hpp"

namespace LLShader {

constexpr glm::vec3 AxisX{1.f, 0.f, 0.f};
constexpr glm::vec3 AxisY{0.f, 1.f, 0.f};
constexpr glm::vec3 AxisZ{0.f, 0.f, 1.f};

// our game coords, the same as vulkan & unity, foward is AxisZ.
constexpr auto world_up = AxisY;
constexpr auto world_front = AxisZ;
constexpr auto world_right = AxisX;

// key map to GameCommand
enum class CameraCommand : u_int32_t {
  camera_move_forward = (u_int32_t)GameCommand::forward,
  camera_move_back = (u_int32_t)GameCommand::backward,
  camera_move_left = (u_int32_t)GameCommand::left,
  camera_move_right = (u_int32_t)GameCommand::right,
};

/// FPS style camera, using eular angle.
class FPSCamera {
 public:
  FPSCamera() = default;
  void move(glm::vec3& delta);
  void rotate(glm::vec3 rotate);
  glm::vec3 processKeyCommand(u_int32_t cmd);

  inline glm::vec3 getPosition() const { return position; }
  inline glm::vec3 getForward() const { return forward; }
  inline glm::vec3 getUp() const { return up; }
  inline glm::vec3 getRight() const { return right; }

  glm::mat4 getViewMatrix() const;
  glm::mat4 getPerspectiveProjectionMatrix() const;

  bool flip_y{false};

  float cursor_sensitive{0.5f};
  float move_speed{0.1f};

  float aspect{16.f / 9.f};
  float fov{45.f};
  float zfar{500.f};
  float znear{0.03f};

  glm::vec3 forward{world_front};
  glm::vec3 up{world_up};
  glm::vec3 right{world_right};
  float current_pitch_degree{0.f};
  float current_yaw_degree{0.f};

  glm::vec3 position{0.f, 0.f, -20.f};
};

/// A camera never constraint direction rotate.
class RenderCamera {
 public:
  RenderCamera() = default;

  /// rotate is degree.
  void rotate(glm::vec3 rotate);

  /// move to
  glm::vec3 processKeyCommand(u_int32_t cmd);

  void move(glm::vec3& delta);

  inline glm::vec3 getPosition() const { return position; }
  inline glm::vec3 getForward() const { return rotation * forward; }
  inline glm::vec3 getUp() const { return rotation * up; }
  inline glm::vec3 getRight() const { return rotation * right; }

  glm::mat4 getViewMatrix() const;
  glm::mat4 getPerspectiveProjectionMatrix() const;

  glm::vec3 forward{world_front};
  glm::vec3 up{world_up};
  glm::vec3 right{world_right};

 private:
  bool flip_y{true};

  float cursor_sensitive{0.5f};
  float move_speed{0.1f};

  float aspect{16.f / 9.f};
  float fov{45.f};  // TODO: cal by aspect
  float zfar{500.f};
  float znear{0.03f};

  glm::vec3 position{0.f, 0.f, 0.f};

  glm::quat rotation{1.f, 0.f, 0.f, 0.f};
};

}  // namespace LLShader

#endif