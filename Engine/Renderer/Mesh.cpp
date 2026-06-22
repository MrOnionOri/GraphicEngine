#include "Engine/Renderer/Mesh.h"

#include <cstddef>

namespace Engine {

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
    GLenum primitive)
    : indexCount_(static_cast<GLsizei>(indices.size())), primitive_(primitive) {
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);
    glGenBuffers(1, &instanceBuffer_);
    glBindVertexArray(vertexArray_);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, textureCoordinate)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, textureIndex)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer_);
    for (GLuint column = 0; column < 4; ++column) {
        const GLuint location = 4 + column;
        glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
            reinterpret_cast<void*>(static_cast<std::size_t>(column) * sizeof(glm::vec4)));
        glEnableVertexAttribArray(location);
        glVertexAttribDivisor(location, 1);
    }
    glBindVertexArray(0);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &instanceBuffer_);
    glDeleteBuffers(1, &indexBuffer_);
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteVertexArrays(1, &vertexArray_);
}

void Mesh::updateGeometry(const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices) {
    indexCount_ = static_cast<GLsizei>(indices.size());
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
}

std::unique_ptr<Mesh> Mesh::createCube() {
    const glm::vec2 uv0{0.0f, 0.0f}, uv1{1.0f, 0.0f}, uv2{1.0f, 1.0f}, uv3{0.0f, 1.0f};
    const std::vector<Vertex> vertices{
        {{-0.5f,-0.5f, 0.5f},{0,0,1},uv0}, {{ 0.5f,-0.5f, 0.5f},{0,0,1},uv1},
        {{ 0.5f, 0.5f, 0.5f},{0,0,1},uv2}, {{-0.5f, 0.5f, 0.5f},{0,0,1},uv3},
        {{ 0.5f,-0.5f,-0.5f},{0,0,-1},uv0}, {{-0.5f,-0.5f,-0.5f},{0,0,-1},uv1},
        {{-0.5f, 0.5f,-0.5f},{0,0,-1},uv2}, {{ 0.5f, 0.5f,-0.5f},{0,0,-1},uv3},
        {{-0.5f,-0.5f,-0.5f},{-1,0,0},uv0}, {{-0.5f,-0.5f, 0.5f},{-1,0,0},uv1},
        {{-0.5f, 0.5f, 0.5f},{-1,0,0},uv2}, {{-0.5f, 0.5f,-0.5f},{-1,0,0},uv3},
        {{ 0.5f,-0.5f, 0.5f},{1,0,0},uv0}, {{ 0.5f,-0.5f,-0.5f},{1,0,0},uv1},
        {{ 0.5f, 0.5f,-0.5f},{1,0,0},uv2}, {{ 0.5f, 0.5f, 0.5f},{1,0,0},uv3},
        {{-0.5f,-0.5f,-0.5f},{0,-1,0},uv0}, {{ 0.5f,-0.5f,-0.5f},{0,-1,0},uv1},
        {{ 0.5f,-0.5f, 0.5f},{0,-1,0},uv2}, {{-0.5f,-0.5f, 0.5f},{0,-1,0},uv3},
        {{-0.5f, 0.5f, 0.5f},{0,1,0},uv0}, {{ 0.5f, 0.5f, 0.5f},{0,1,0},uv1},
        {{ 0.5f, 0.5f,-0.5f},{0,1,0},uv2}, {{-0.5f, 0.5f,-0.5f},{0,1,0},uv3}
    };
    const std::vector<unsigned int> indices{
        0,1,2, 2,3,0, 4,5,6, 6,7,4, 8,9,10, 10,11,8,
        12,13,14, 14,15,12, 16,17,18, 18,19,16, 20,21,22, 22,23,20
    };
    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createWireCube() {
    const std::vector<Vertex> vertices{
        {{-0.5f,-0.5f,-0.5f}}, {{ 0.5f,-0.5f,-0.5f}},
        {{ 0.5f, 0.5f,-0.5f}}, {{-0.5f, 0.5f,-0.5f}},
        {{-0.5f,-0.5f, 0.5f}}, {{ 0.5f,-0.5f, 0.5f}},
        {{ 0.5f, 0.5f, 0.5f}}, {{-0.5f, 0.5f, 0.5f}}
    };
    const std::vector<unsigned int> indices{
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };
    return std::make_unique<Mesh>(vertices, indices, GL_LINES);
}

void Mesh::uploadInstances(const std::vector<glm::mat4>& transforms) {
    instanceCount_ = static_cast<GLsizei>(transforms.size());
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(transforms.size() * sizeof(glm::mat4)),
        transforms.data(), GL_DYNAMIC_DRAW);
}

void Mesh::drawInstanced() const {
    glBindVertexArray(vertexArray_);
    glDrawElementsInstanced(primitive_, indexCount_, GL_UNSIGNED_INT, nullptr, instanceCount_);
    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(vertexArray_);
    glDrawElements(primitive_, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace Engine
