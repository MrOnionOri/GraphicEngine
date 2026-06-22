#include "Engine/Renderer/Shader.h"
#include "Engine/Core/AssetPath.h"

#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Engine {

Shader::Shader(const char* vertexSource, const char* fragmentSource) {
    createProgram(vertexSource, fragmentSource);
}

Shader::Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
    const std::string vertexSource = readAsset(vertexPath);
    const std::string fragmentSource = readAsset(fragmentPath);
    createProgram(vertexSource.c_str(), fragmentSource.c_str());
}

void Shader::createProgram(const char* vertexSource, const char* fragmentSource) {
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = 0;
    try {
        fragment = compile(GL_FRAGMENT_SHADER, fragmentSource);
        id_ = glCreateProgram();
        glAttachShader(id_, vertex);
        glAttachShader(id_, fragment);
        glLinkProgram(id_);
        GLint linked = GL_FALSE;
        glGetProgramiv(id_, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) throw std::runtime_error("Shader link: " + programLog(id_));
    } catch (...) {
        glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        if (id_) glDeleteProgram(id_);
        throw;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() { if (id_) glDeleteProgram(id_); }
void Shader::bind() const { glUseProgram(id_); }

void Shader::setMat4(const char* name, const glm::mat4& value) const {
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) const {
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setFloat(const char* name, float value) const {
    glUniform1f(uniformLocation(name), value);
}

void Shader::setInt(const char* name, int value) const {
    glUniform1i(uniformLocation(name), value);
}

void Shader::setUInt(const char* name, unsigned int value) const {
    glUniform1ui(uniformLocation(name), value);
}

GLint Shader::uniformLocation(const char* name) const {
    const auto existing = uniformLocations_.find(name);
    if (existing != uniformLocations_.end()) return existing->second;
    const GLint location = glGetUniformLocation(id_, name);
    if (location < 0) throw std::runtime_error(std::string("Uniform no encontrado: ") + name);
    uniformLocations_.emplace(name, location);
    return location;
}

GLuint Shader::compile(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        const std::string log = shaderLog(shader);
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile: " + log);
    }
    return shader;
}

std::string Shader::shaderLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string result(static_cast<std::size_t>(std::max(1, length)), '\0');
    glGetShaderInfoLog(shader, length, nullptr, result.data());
    return result;
}

std::string Shader::programLog(GLuint program) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string result(static_cast<std::size_t>(std::max(1, length)), '\0');
    glGetProgramInfoLog(program, length, nullptr, result.data());
    return result;
}

std::string Shader::readAsset(const std::filesystem::path& path) {
    const std::filesystem::path resolved = AssetPath::resolve(path);
    std::ifstream stream(resolved, std::ios::binary);
    if (!stream) throw std::runtime_error("No se pudo abrir el asset: " + resolved.string());
    std::ostringstream contents;
    contents << stream.rdbuf();
    return contents.str();
}

} // namespace Engine
