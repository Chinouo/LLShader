#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <cstdio>
#include <memory>
#include <thread>

#include "demos/pcss/pcss.hpp"

namespace LLShader {

class TrianglePipeline;
class InputManager;
class WindowManager;
class VkHolder;
class RenderManager;

class Matrix final {
 public:
  Matrix();

  void init();

  void run();

  void shutdown();

  ~Matrix();

 public:
  std::shared_ptr<WindowManager> window_manager;
  std::shared_ptr<InputManager> input_manager;
  std::shared_ptr<VkHolder> vk_holder;
  std::shared_ptr<RenderManager> render_manager;

 private:
  /// render loop
  inline void main_loop();
  inline void runOnceDuringEachLoopBegin();
  inline void runOnceDuringEachLoopEnd();

  // std::shared_ptr<VkHolder> vkHolder_;
  // std::unique_ptr<TrianglePipeline> gPipelineOwner_;
  // std::unique_ptr<PcssPipeline> pcss_pipeline;
  //        GLFWwindow* window;

  std::chrono::steady_clock::time_point last_tick_time_point{
      std::chrono::steady_clock::now()};
};

extern Matrix global_matrix_engine;

}  // namespace LLShader

#endif