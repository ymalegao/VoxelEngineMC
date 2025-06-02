#ifndef IMGUI_HANDLER_HPP
#define IMGUI_HANDLER_HPP

#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#define ENSURE_MAIN_THREAD() \
    do { \
        static const auto mainThreadId = std::this_thread::get_id(); \
        assert(std::this_thread::get_id() == mainThreadId && "ImGui used from non-main thread!"); \
    } while(0)

class ImGuiHandler {
public:
    static void Initialize(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    static void BeginFrame() {
        ENSURE_MAIN_THREAD();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    static void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    static void Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
};

#endif // IMGUI_HANDLER_HPP
