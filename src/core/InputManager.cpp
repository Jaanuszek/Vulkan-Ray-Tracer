#include "pch.h"
#include "InputManager.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"

namespace VRTR
{
    std::unordered_map<int, bool> InputManager::keyStates;
    std::unordered_map<int, bool> InputManager::mouseButtonStates;
    std::unordered_map<int, bool> InputManager::keyStatesLastFrame;
    double InputManager::mouseX{};
    double InputManager::mouseY{};
    double InputManager::lastMouseX{};
    double InputManager::lastMouseY{};
    bool InputManager::firstMouse = true;
    bool InputManager::renderGUI = false;

    void InputManager::init(GLFWwindow *window)
    {
        glfwSetKeyCallback(window, key_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetCharCallback(window, char_callback);
        setCursorCallback(window);
    }

    bool InputManager::isKeyPressed(int key)
    {
        auto it = keyStates.find(key);
        return it != keyStates.end() && it->second;
    }

    bool InputManager::isKeyJustPressed(int key)
    {
        bool isPressed = isKeyPressed(key);
        bool wasPressedLastFrame = keyStatesLastFrame[key];

        keyStatesLastFrame[key] = isPressed;

        return isPressed && !wasPressedLastFrame;
    }

    bool InputManager::isMouseButtonPressed(int button)
    {
        auto it = mouseButtonStates.find(button);
        return it != mouseButtonStates.end() && it->second;
    }

    void InputManager::setCursorCallback(GLFWwindow* window) 
    { 
        glfwSetCursorPosCallback(window, mouse_callback); 
    }
    void InputManager::disableCursorCallback(GLFWwindow* window) 
    { 
        glfwSetCursorPosCallback(window, nullptr);
        InputManager::firstMouse = true; 
    }

    void InputManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

        if (action == GLFW_PRESS)
            keyStates[key] = true;
        else if (action == GLFW_RELEASE)
            keyStates[key] = false;
    }

    void InputManager::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

        if (action == GLFW_PRESS)
            mouseButtonStates[button] = true;
        else if (action == GLFW_RELEASE)
            mouseButtonStates[button] = false;
    }

    void InputManager::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    }

    void InputManager::char_callback(GLFWwindow* window, unsigned int c)
    {
        ImGui_ImplGlfw_CharCallback(window, c);
    }

    void InputManager::mouse_callback(GLFWwindow* window, double xpos, double ypos)
    {
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

        const bool imguiCapturesMouse =
            InputManager::renderGUI &&
            (ImGui::GetCurrentContext() != nullptr) &&
            ImGui::GetIO().WantCaptureMouse;
        if (imguiCapturesMouse)
        {
            lastMouseX = xpos;
            lastMouseY = ypos;
            return;
        }

        if (firstMouse)
        {
            lastMouseX = xpos;
            lastMouseY = ypos;
            firstMouse = false;
        }

        double xoffset = xpos - lastMouseX;
        double yoffset = ypos - lastMouseY;
        
        lastMouseX = xpos;
        lastMouseY = ypos;

        Camera* camera = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
        assert(camera && "Camera pointer is null in mouse_callback");
        
        camera->ProcessMouseMovement(static_cast<float>(xoffset), static_cast<float>(yoffset));
    }

    void processInput(GLFWwindow* window, double deltaTime, std::shared_ptr<Camera> camera)
    {
        const bool imguiCapturesKeyboard =
            InputManager::renderGUI &&
            (ImGui::GetCurrentContext() != nullptr) &&
            ImGui::GetIO().WantCaptureKeyboard;

        float cameraSpeed = 2.5f * static_cast<float>(deltaTime);

        if(!imguiCapturesKeyboard)
        {
            if (InputManager::isKeyPressed(ActionKeyMap[Action::CloseWindow]))
            {
                glfwSetWindowShouldClose(window, true);
            }

            if(InputManager::isKeyPressed(ActionKeyMap[Action::MoveForward]))
            {
                camera->moveForward(cameraSpeed);
            }
            if(InputManager::isKeyPressed(ActionKeyMap[Action::MoveBackward]))
            {
                camera->moveForward(-cameraSpeed);
            }
            if(InputManager::isKeyPressed(ActionKeyMap[Action::MoveLeft]))
            {
                camera->moveRight(-cameraSpeed);
            }
            if(InputManager::isKeyPressed(ActionKeyMap[Action::MoveRight]))
            {
                camera->moveRight(cameraSpeed);
            }
            if(InputManager::isKeyPressed(ActionKeyMap[Action::MoveUp]))
            {
                camera->moveUp(cameraSpeed);
            }
            if(InputManager::isKeyPressed(ActionKeyMap[Action::MoveDown]))
            {
                camera->moveUp(-cameraSpeed);
            }
        }

        if(InputManager::isKeyPressed(ActionKeyMap[Action::EnableMouse]))
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            InputManager::disableCursorCallback(window);
        }
        if(InputManager::isKeyPressed(ActionKeyMap[Action::DisableMouse]))
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            InputManager::setCursorCallback(window);
        }

        if(InputManager::isKeyJustPressed(ActionKeyMap[Action::ToggleGUI]))
        {
            InputManager::renderGUI = !InputManager::renderGUI;
        }
    }
}