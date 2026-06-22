#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <cstdint>
#include <vector>

namespace Engine {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 textureCoordinate;
    float textureIndex = 0.0f;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
        GLenum primitive = GL_TRIANGLES);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    static std::unique_ptr<Mesh> createCube();
    static std::unique_ptr<Mesh> createWireCube();
    void updateGeometry(const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices);
    void uploadInstances(const std::vector<glm::mat4>& transforms);
    void drawInstanced() const;
    void draw() const;
    std::uint64_t triangleCount() const { return static_cast<std::uint64_t>(indexCount_) / 3; }
    GLsizei instanceCount() const { return instanceCount_; }

private:
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLuint instanceBuffer_ = 0;
    GLsizei indexCount_ = 0;
    GLsizei instanceCount_ = 0;
    GLenum primitive_ = GL_TRIANGLES;
};

} // namespace Engine
