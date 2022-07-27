#include "input_manager.hpp"

#include "engine/matrix.hpp"
#include "render/window_manager.hpp"

namespace LLShader {

void InputManager::recordKeys(GLFWwindow* wd, int key, int scancode, int action,
                              int mode) {
  if (global_matrix_engine.window_manager->isEditMode()) {
    return;
  }
  global_matrix_engine.input_manager->onKeyEvent(
      KeyEvent{key, scancode, action, mode});
}

void InputManager::recordCursorPos(GLFWwindow* wd, double position_x,
                                   double position_y) {
  if (global_matrix_engine.window_manager->isEditMode()) {
    return;
  }
  global_matrix_engine.input_manager->onCursorMove(
      CursorPosEvent{position_x, position_y});
}

void InputManager::init() {
  m_wd = global_matrix_engine.window_manager;

  if (m_wd->isEditMode()) {
  } else {
    glfwSetInputMode(m_wd->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetKeyCallback(m_wd->window, InputManager::recordKeys);
    glfwSetCursorPosCallback(m_wd->window, InputManager::recordCursorPos);
  }
}

void InputManager::dispose() {
  //
}

// in edit mode, this func never called.
void InputManager::onKeyEvent(KeyEvent event) {
  if (event.action == GLFW_PRESS) {
    switch (event.key) {
      case GLFW_KEY_W:
        current_command_state |= (u_int32_t)GameCommand::forward;
        break;
      case GLFW_KEY_A:
        current_command_state |= (u_int32_t)GameCommand::left;
        break;
      case GLFW_KEY_S:
        current_command_state |= (u_int32_t)GameCommand::backward;
        break;
      case GLFW_KEY_D:
        current_command_state |= (u_int32_t)GameCommand::right;
        break;
      case GLFW_KEY_ESCAPE:
        // send message to window_manager rather than be a notifier.
        current_command_state |= (u_int32_t)GameCommand::unfocus;
        global_matrix_engine.window_manager->setWindowMode(WindowMode::edit);
        resetEscBit();
        break;
      case GLFW_KEY_ENTER:
        current_command_state |= (u_int32_t)GameCommand::focus;
        break;
      default:
        break;
    }

  } else if (event.action == GLFW_RELEASE) {
    switch (event.key) {
      case GLFW_KEY_W:
        current_command_state &=
            (u_int32_t)GameCommand::forward ^ k_complete_command;
        break;
      case GLFW_KEY_A:
        current_command_state &=
            (u_int32_t)GameCommand::left ^ k_complete_command;
        break;
      case GLFW_KEY_S:
        current_command_state &=
            (u_int32_t)GameCommand::backward ^ k_complete_command;
        break;
      case GLFW_KEY_D:
        current_command_state &=
            (u_int32_t)GameCommand::right ^ k_complete_command;
        break;
      case GLFW_KEY_ESCAPE:  // never go here, because we give callback to
                             // imgui, so GLFW_ESCAPE PRESSED never captured.
        current_command_state &=
            (u_int32_t)GameCommand::unfocus ^ k_complete_command;
        break;
      case GLFW_KEY_ENTER:
        current_command_state &=
            (u_int32_t)GameCommand::focus ^ k_complete_command;
        break;
      default:
        break;
    }
  }

  // key_events_queue.push_back(key);
}

void InputManager::onCursorMove(CursorPosEvent event) {
  //   if (is_first_cursor_event) {
  //     current_cursor_position = event;
  //     last_cursor_position = event;
  //     is_first_cursor_event = false;
  //   }
  //   last_cursor_position = current_cursor_position;
  //   current_cursor_position = event;
  //   current_cursor_state = event;
}

void InputManager::updateCursorMetrices() {
  if (m_wd->isEditMode()) {
    is_first_cursor_event = true;
    current_cursor_position = last_cursor_position = {0.0, 0.0};
    return;
  };

  double x;
  double y;
  glfwGetCursorPos(m_wd->window, &x, &y);

  if (is_first_cursor_event) {
    current_cursor_position = {x, y};
    is_first_cursor_event = false;
  }
  last_cursor_position = current_cursor_position;
  current_cursor_position = {x, y};
}

void InputManager::installGlfwCallBacks(GLFWwindow* window) {
  glfwSetKeyCallback(window, InputManager::recordKeys);
  glfwSetCursorPosCallback(window, InputManager::recordCursorPos);
}

}  // namespace LLShader