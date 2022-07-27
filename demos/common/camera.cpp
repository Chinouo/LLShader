#include "camera.hpp"

namespace LLShader {

void FPSCamera::rotate(glm::vec3 rotate) {
  rotate *= cursor_sensitive;
  if (flip_y) rotate.x *= -1;
  current_pitch_degree =
      glm::clamp(current_pitch_degree + rotate.x, -89.f, 89.f);
  current_yaw_degree += rotate.y;

  // sequence: yaw -> pitch
  auto pitch = glm::radians(current_pitch_degree);
  auto yaw = glm::radians(current_yaw_degree);
  forward.x = glm::sin(yaw) * glm::cos(pitch);
  forward.y = glm::sin(pitch);
  forward.z = glm::cos(yaw) * glm::cos(pitch);
  forward = glm::normalize(forward);

#ifdef GLM_FORCE_LEFT_HANDED
  // left hand cross
  right = glm::cross(world_up, forward);
  up = glm::cross(forward, right);
#else
  right = glm::cross(forward, world_up);
  up = glm::cross(right, forward);
#endif
}

void FPSCamera::move(glm::vec3& delta) { position += delta; }

glm::vec3 FPSCamera::processKeyCommand(u_int32_t cmd) {
  glm::vec3 delta{0.f, 0.f, 0.f};
  auto front = getForward();
  auto right = getRight();
  if (cmd & (u_int32_t)CameraCommand::camera_move_forward) {
    delta += front * move_speed;
  }

  if (cmd & (u_int32_t)CameraCommand::camera_move_back) {
    delta -= front * move_speed;
  }

  if (cmd & (u_int32_t)CameraCommand::camera_move_left) {
    delta -= right * move_speed;
  }

  if (cmd & (u_int32_t)CameraCommand::camera_move_right) {
    delta += right * move_speed;
  }
  return delta;
}

glm::mat4 FPSCamera::getViewMatrix() const {
  return glm::lookAt(position, position + getForward(), getUp());
}

glm::mat4 FPSCamera::getPerspectiveProjectionMatrix() const {
  return glm::perspective(glm::radians(fov), aspect, znear, zfar);
}

// render camera
void RenderCamera::rotate(glm::vec3 rotate) {
  // rotate : x(pitch) y(yaw) z(roll)
  if (flip_y) rotate.x *= -1;
  auto pitch = glm::radians(rotate.x);
  auto yaw = glm::radians(rotate.y);
  auto pitch_quat = glm::angleAxis(pitch, AxisX);
  auto yaw_quat = glm::angleAxis(yaw, AxisY);

  rotation = rotation * pitch_quat * yaw_quat;
}

void RenderCamera::move(glm::vec3& delta) { position += delta; }

glm::vec3 RenderCamera::processKeyCommand(u_int32_t cmd) {
  glm::vec3 delta{0.f, 0.f, 0.f};
  auto front = getForward();
  auto right = getRight();
  if (cmd & (u_int32_t)CameraCommand::camera_move_forward) {
    delta += front * move_speed;
  }

  if (cmd & (u_int32_t)CameraCommand::camera_move_back) {
    delta -= front * move_speed;
  }

  if (cmd & (u_int32_t)CameraCommand::camera_move_left) {
    delta -= right * move_speed;
  }

  if (cmd & (u_int32_t)CameraCommand::camera_move_right) {
    delta += right * move_speed;
  }
  return delta;
}

glm::mat4 RenderCamera::getPerspectiveProjectionMatrix() const {
  return glm::perspective(glm::radians(fov), aspect, znear, zfar);
}

glm::mat4 RenderCamera::getViewMatrix() const {
  return glm::lookAt(position, position + getForward(), getUp());
}

}  // namespace LLShader