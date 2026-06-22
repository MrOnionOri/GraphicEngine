#pragma once

#include <GL/glew.h>
#include <filesystem>
#include <memory>

namespace Engine {

class Texture2D {
public:
    Texture2D(int width, int height, const unsigned char* pixels);
    ~Texture2D();
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    static Texture2D checkerboard();
    static std::unique_ptr<Texture2D> loadPpm(const std::filesystem::path& path);
    void bind(unsigned int slot = 0) const;

private:
    GLuint id_ = 0;
};

} // namespace Engine
