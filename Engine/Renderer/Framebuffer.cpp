#include "Engine/Renderer/Framebuffer.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace Engine {

Framebuffer::Framebuffer(int width, int height) : width_(width), height_(height) { invalidate(); }

Framebuffer::~Framebuffer() {
    glDeleteRenderbuffers(1, &depthAttachment_);
    glDeleteTextures(1, &colorAttachment_);
    glDeleteTextures(1, &entityIdAttachment_);
    glDeleteFramebuffers(1, &id_);
}

std::uint32_t Framebuffer::readEntityId(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return 0;
    glBindFramebuffer(GL_FRAMEBUFFER, id_);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    std::uint32_t entityId = 0;
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &entityId);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    return entityId;
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, id_);
    glViewport(0, 0, width_, height_);
}

void Framebuffer::unbind(int windowWidth, int windowHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
}

void Framebuffer::resize(int width, int height) {
    width = std::clamp(width, 1, 8192);
    height = std::clamp(height, 1, 8192);
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    invalidate();
}

void Framebuffer::invalidate() {
    if (id_) {
        glDeleteRenderbuffers(1, &depthAttachment_);
        glDeleteTextures(1, &colorAttachment_);
        glDeleteTextures(1, &entityIdAttachment_);
        glDeleteFramebuffers(1, &id_);
    }
    glGenFramebuffers(1, &id_);
    glBindFramebuffer(GL_FRAMEBUFFER, id_);

    glGenTextures(1, &colorAttachment_);
    glBindTexture(GL_TEXTURE_2D, colorAttachment_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment_, 0);

    glGenTextures(1, &entityIdAttachment_);
    glBindTexture(GL_TEXTURE_2D, entityIdAttachment_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width_, height_, 0, GL_RED_INTEGER,
        GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
        entityIdAttachment_, 0);

    constexpr GLenum drawBuffers[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBuffers);

    glGenRenderbuffers(1, &depthAttachment_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthAttachment_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthAttachment_);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("No se pudo crear el framebuffer del viewport");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace Engine
