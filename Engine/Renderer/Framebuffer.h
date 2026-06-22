#pragma once

#include <GL/glew.h>
#include <cstdint>

namespace Engine {

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void bind() const;
    static void unbind(int windowWidth, int windowHeight);
    void resize(int width, int height);
    std::uint32_t readEntityId(int x, int y) const;

    GLuint colorAttachment() const { return colorAttachment_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    GLuint id_ = 0;
    GLuint colorAttachment_ = 0;
    GLuint entityIdAttachment_ = 0;
    GLuint depthAttachment_ = 0;
    int width_ = 0;
    int height_ = 0;
    void invalidate();
};

} // namespace Engine
