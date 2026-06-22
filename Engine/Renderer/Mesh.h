#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Engine {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 textureCoordinate;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    static std::unique_ptr<Mesh> createCube();
    void uploadInstances(const std::vector<glm::mat4>& transforms);
    void drawInstanced() const;

private:
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLuint instanceBuffer_ = 0;
    GLsizei indexCount_ = 0;
    GLsizei instanceCount_ = 0;
};

} // namespace Engine
