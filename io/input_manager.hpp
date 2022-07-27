#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include <memory>
#include <vector>

#include "3rd/glfw/include/GLFW/glfw3.h"
#include "util/observer.hpp"
// #include "engine/matrix.hpp"
#include "render/window_manager.hpp"

namespace LLShader {

const u_int32_t k_complete_command = 0xFFFFFFFF;

enum class GameCommand : u_int32_t {
  forward = 1 << 0,   // W
  backward = 1 << 1,  // S
  left = 1 << 2,      // A
  right = 1 << 3,     // D
  unfocus = 1 << 4,   // press ESC to quit focus
  focus = 1 << 5,     // press enter to enter focus
  undefined = 1 << 0,
};

typedef struct {
  int key;
  int scancode;
  int action;
  int mods;
} KeyEvent;

typedef struct {
  double x;
  double y;
} CursorPosEvent;

typedef void (*CursorMoveCallBack)(CursorPosEvent);

class InputManager final : public ChangeNotifier {
 public:
  static void recordKeys(GLFWwindow* wd, int key, int scancode, int action,
                         int mode);
  static void recordCursorPos(GLFWwindow* wd, double position_x,
                              double position_y);

  InputManager() = default;
  InputManager(const InputManager&) = delete;

  void init();
  void dispose();

  /// 向 GLFW 注册输入回掉函数，Imgui会覆盖，懒得再写一个restore 了
  void installGlfwCallBacks(GLFWwindow* window);

  void onKeyEvent(KeyEvent event);

  // TODO: remove this func;
  void onCursorMove(CursorPosEvent event);

  // 参数应该为函数指针
  // void registerKeyEventCallBack();
  // void registerCursorMoveCallBack();
  // void registerKeyEventCallBack();

  // void registerCursorMoveCallBack();

  // only called when change mode
  void resetEscBit() {
    current_command_state &=
        (u_int32_t)GameCommand::unfocus ^ k_complete_command;
  }

  inline u_int32_t getCurrentCommandState() { return current_command_state; }

  inline CursorPosEvent getCurrentCursorPositionState() {
    return current_cursor_state;
  }

  void updateCursorMetrices();

  inline CursorPosEvent getCursorPositionDelta() {
    return {current_cursor_position.x - last_cursor_position.x,
            current_cursor_position.y - last_cursor_position.y};
  }

 private:
  u_int32_t current_command_state{0};
  bool is_first_cursor_event{true};
  CursorPosEvent last_cursor_position;
  CursorPosEvent current_cursor_position;
  CursorPosEvent current_cursor_state{0.0, 0.0};

  // non-local
  std::shared_ptr<WindowManager> m_wd;
};

}  // namespace LLShader

#endif