#include <exception>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT

#define STB_IMAGE_IMPLEMENTATION
#include "engine/matrix.hpp"
#include "glm/gtc/quaternion.hpp"
#include "log/log.hpp"
#include "stb_image.h"

using namespace LLShader;

int main() {
  // glm::vec4 v(1.f, 0.f, 0.f, 0.f);
  // glm::qua q{glm::radians(glm::vec3(0.f, 0.f, 90.f))};
  // auto res = glm::mat4_cast(q) * v;
  // float x = v.x;

  auto origin_state = glm::qua(1.f, 0.f, 0.f, 0.f);
  auto pitch = glm::angleAxis(glm::radians(00.f), glm::vec3(1.f, 0.f, 0.f));
  auto yaw = glm::angleAxis(glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));

  glm::vec4 p{1.f, 0.f, 0.f, 1.f};
  auto res = glm::mat4_cast(origin_state * pitch * yaw) * p;

  try {
    global_matrix_engine.init();
    global_matrix_engine.run();
    global_matrix_engine.shutdown();
  } catch (std::exception &e) {
    LogUtil::LogE(e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
