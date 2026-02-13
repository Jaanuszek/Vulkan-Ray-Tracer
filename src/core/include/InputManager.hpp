#pragma once

#include "Logger.hpp"
#include "Camera.hpp"

namespace VRTR
{
    enum class Action
    {
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        CloseWindow,
        EnableMouse, // bring back the os mouse
        DisableMouse, // hide Os mouse and use camera

        ACTION_COUNT // last element, dont overwrite it
    };

    static std::unordered_map<Action, int> ActionKeyMap = {
        {Action::MoveForward, GLFW_KEY_W},
        {Action::MoveBackward, GLFW_KEY_S},
        {Action::MoveLeft, GLFW_KEY_A},
        {Action::MoveRight, GLFW_KEY_D},
        {Action::MoveUp, GLFW_KEY_SPACE},
        {Action::MoveDown, GLFW_KEY_LEFT_CONTROL},
        {Action::CloseWindow, GLFW_KEY_ESCAPE},
        {Action::EnableMouse, GLFW_KEY_F1},
        {Action::DisableMouse, GLFW_KEY_F2},
    };

    class CORE_EXPORT InputManager
    {
        public:
            static void init(GLFWwindow *window);
            static bool isKeyPressed(int key);
            static bool isMouseButtonPressed(int button);
            static void setCursorCallback(GLFWwindow *window);
            static void disableCursorCallback(GLFWwindow *window);

        private:
            static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
            // static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
            static void mouse_callback(GLFWwindow* window, double xpos, double ypos);


        private:
            static std::unordered_map<int, bool> keyStates;
            static std::unordered_map<int, bool> mouseButtonStates;
            static double mouseX, mouseY;
            static double lastMouseX, lastMouseY;
            static bool firstMouse;
    };

    void CORE_EXPORT processInput(GLFWwindow* window, double deltaTime, std::shared_ptr<Camera> camera);
    // void CORE_EXPORT mouseCallback(GLFWwindow* window, double xpos, double ypos);
}