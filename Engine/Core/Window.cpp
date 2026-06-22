#include "Engine/Core/Window.h"

#include <iostream>
#include <stdexcept>

namespace Engine {

Window::Window(int width, int height, std::string title)
    : width_(width), height_(height), title_(std::move(title)) {
    glfwSetErrorCallback([](int code, const char* description) {
        std::cerr << "GLFW (" << code << "): " << description << '\n';
    });
    if (glfwInit() != GLFW_TRUE) throw std::runtime_error("No se pudo inicializar GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    handle_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if (!handle_) {
        glfwTerminate();
        throw std::runtime_error("No se pudo crear la ventana");
    }

    glfwMakeContextCurrent(handle_);
    glfwSetWindowUserPointer(handle_, this);
    glfwSetFramebufferSizeCallback(handle_, framebufferSizeCallback);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    const GLenum status = glewInit();
    if (status != GLEW_OK) {
        glfwDestroyWindow(handle_);
        handle_ = nullptr;
        glfwTerminate();
        throw std::runtime_error("No se pudo inicializar GLEW");
    }
    while (glGetError() != GL_NO_ERROR) {}

    glfwGetFramebufferSize(handle_, &width_, &height_);
    glViewport(0, 0, width_, height_);
}

Window::~Window() {
    if (handle_) glfwDestroyWindow(handle_);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(handle_) == GLFW_TRUE; }
void Window::requestClose() { glfwSetWindowShouldClose(handle_, GLFW_TRUE); }
void Window::pollEvents() const { glfwPollEvents(); }
void Window::swapBuffers() const { glfwSwapBuffers(handle_); }

float Window::aspectRatio() const {
    return height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;
}

void Window::framebufferSizeCallback(GLFWwindow* handle, int width, int height) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window) {
        window->width_ = width;
        window->height_ = height;
    }
    glViewport(0, 0, width, height);
}

} // namespace Engine
