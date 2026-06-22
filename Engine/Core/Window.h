#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

namespace Engine {

class Window {
public:
    Window(int width, int height, std::string title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void requestClose();
    void pollEvents() const;
    void swapBuffers() const;

    int width() const { return width_; }
    int height() const { return height_; }
    float aspectRatio() const;
    GLFWwindow* nativeHandle() const { return handle_; }

private:
    GLFWwindow* handle_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    std::string title_;

    static void framebufferSizeCallback(GLFWwindow* handle, int width, int height);
};

} // namespace Engine
