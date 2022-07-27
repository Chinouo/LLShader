#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <stdexcept>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN 
#endif
#include "3rd/glfw/include/GLFW/glfw3.h"
#include "3rd/imgui/imgui.h"
#include "3rd/imgui/backends/imgui_impl_glfw.h"

#include "vulkan/vulkan.hpp"

#include "log/log.hpp"
#include "util/observer.hpp"

namespace LLShader
{

/// 设置 编辑 和 纯预览模式
enum WindowMode{
    edit,
    play,
};

class WindowManager : public Listener{
 public:

    friend class Matrix;

    WindowManager() = default;

    WindowManager(const WindowManager&) = delete;

    ~WindowManager();

    void setWindowMode(WindowMode mode);

    inline bool isEditMode() const{
        return current_mode_ == edit;
    }

    void switchWindowMode(WindowMode target);

    void onNotification() override;

 public:
    GLFWwindow *window;

 private:
    void init(const int width, const int height);
    void dispose();

    // void createWSI();

    WindowMode current_mode_ {edit};

};
    
} // namespace LLShader



#endif