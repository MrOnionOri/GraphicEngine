#pragma once

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Engine {

class Shader {
public:
    Shader(const char* vertexSource, const char* fragmentSource);
    Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void bind() const;
    void setMat4(const char* name, const glm::mat4& value) const;
    void setVec3(const char* name, const glm::vec3& value) const;
    void setFloat(const char* name, float value) const;
    void setInt(const char* name, int value) const;
    void setUInt(const char* name, unsigned int value) const;

private:
    GLuint id_ = 0;
    mutable std::unordered_map<std::string, GLint> uniformLocations_;
    GLint uniformLocation(const char* name) const;
    static GLuint compile(GLenum type, const char* source);
    static std::string shaderLog(GLuint shader);
    static std::string programLog(GLuint program);
    static std::string readAsset(const std::filesystem::path& path);
    void createProgram(const char* vertexSource, const char* fragmentSource);
};

} // namespace Engine
