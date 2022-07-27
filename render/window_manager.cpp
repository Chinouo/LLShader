#include "window_manager.hpp"

#include "io/input_manager.hpp"
#include "render/vk_context.hpp"
#include "engine/matrix.hpp"

namespace LLShader
{

    void WindowManager::init(const int width, const int height){
        if(glfwInit() != GLFW_TRUE){
            throw std::runtime_error("glfwInit Failed\n");
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "LLShader", nullptr, nullptr);
        if(glfwVulkanSupported() != GLFW_TRUE){
            throw std::runtime_error("Vulkan not supported.\n");
        }else{
            LogUtil::LogI("Vulkan Supported.\n");
        }
        
        // TODO: remove desciption
        // listen key event to perform mode change.
        //global_matrix_engine.input_manager->addListener(this);

        assert(current_mode_ == edit);
        if(current_mode_ == edit){

        }



    }

    WindowManager::~WindowManager(){

    }

    void WindowManager::dispose(){
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    // void WindowManager::createWSI(){
    //     glfwCreateWindowSurface(global_matrix_engine.vk_holder->getVkContext().instance,window, 
    //                             nullptr, 
    //                             &surface);
    // }

    void WindowManager::setWindowMode(WindowMode mode){
        if(mode == current_mode_) return;
        switchWindowMode(mode);
    }

    void WindowManager::switchWindowMode(WindowMode target){

        if(target == WindowMode::play){
            // from edit to play
            // hide cursor and close imgui callback.
            // also, we need reset big flag, the next GLFW_PRESS event never 
            // recived by listener, see [ InputManager ] onKey func.
            ImGui_ImplGlfw_RestoreCallbacks(window);
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
            // set input mode once dose'n work, override it after imp_vk_newfame
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);  // TODO: use disabled.
            global_matrix_engine.input_manager->installGlfwCallBacks(window);
            current_mode_ = target;
        }else if(target == WindowMode::edit){
            // from play to edit
            // show cursor and user inputs are processed by imgui.
            ImGui_ImplGlfw_InstallCallbacks(window);
            // iml_vk_newfame will set.
            //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags ^= ImGuiConfigFlags_NoMouse;
            current_mode_ =  target;
        }


    }

    void WindowManager::onNotification(){
       
        u_int32_t cmd_state = global_matrix_engine.input_manager->getCurrentCommandState();
        
        // never call this.
        // if(cmd_state & static_cast<uint32_t>(GameCommand::focus)){
        //     setWindowMode(WindowMode::play);
        // }

        if(cmd_state & static_cast<uint32_t>(GameCommand::unfocus)){
            setWindowMode(WindowMode::edit);
        }

    }

}