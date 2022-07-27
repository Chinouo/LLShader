#include "matrix.hpp"

#include "3rd/imgui/backends/imgui_impl_glfw.h"
#include "3rd/imgui/backends/imgui_impl_vulkan.h"
#include "3rd/imgui/imgui.h"
#include "demos/guitest/gui_test.hpp"
#include "demos/pcss/pcss.hpp"
#include "demos/triangle/triangle.hpp"
#include "io/input_manager.hpp"
#include "render/render_manager.hpp"
#include "render/vk_context.hpp"
#include "render/window_manager.hpp"
#include "util/assets_helper.hpp"

namespace LLShader {

Matrix global_matrix_engine;

Matrix::Matrix() {}

Matrix::~Matrix() {}

void Matrix::init() {
  // follow init sequence.
  window_manager = std::make_shared<WindowManager>();
  vk_holder = std::make_shared<VkHolder>();
  input_manager = std::make_shared<InputManager>();
  render_manager = std::make_shared<RenderManager>();

  window_manager->init(1280, 720);
  vk_holder->init();
  input_manager->init();
  render_manager->init();
}

void Matrix::shutdown() {
  // input_manager->dispose();
  // window_manager->dispose();
  input_manager->dispose();
  render_manager->dispose();
  vk_holder->dispose();
  window_manager->dispose();
}

void Matrix::run() { main_loop(); }

inline void Matrix::main_loop() {
  auto* window = window_manager->window;

  while (!glfwWindowShouldClose(window)) {
    using namespace std::chrono;

    steady_clock::time_point current_time_tick_point = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(
        current_time_tick_point - last_tick_time_point);
    double elapsed = time_span.count();
    last_tick_time_point = current_time_tick_point;

    runOnceDuringEachLoopBegin();

    // 更新 Pipeline 的状态 包含 Pipeline 内部的 IMGUI 状态
    while (elapsed > 0.0) {
      double dt = std::min((1.0 / 120.0), elapsed);
      render_manager->p_current_draw_context->update(dt);
      elapsed -= dt;
    }

    // 渲染 全局的 IMGUI 内容 和 Pipeline 内部自己的 IMGUI内容 和 图像内容
    {
      render_manager->beginFrame();
      render_manager->drawFrame();
      render_manager->endFrame();
    }

    runOnceDuringEachLoopEnd();
  }

  vkDeviceWaitIdle(render_manager->device_);
}

inline void Matrix::runOnceDuringEachLoopBegin() {
  glfwPollEvents();
  input_manager->updateCursorMetrices();
  input_manager->notifyListeners();
}
inline void Matrix::runOnceDuringEachLoopEnd() {}

}  // namespace LLShader