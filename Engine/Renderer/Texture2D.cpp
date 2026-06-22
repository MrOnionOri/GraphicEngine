#include "Engine/Renderer/Texture2D.h"
#include "Engine/Core/AssetPath.h"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Engine {

Texture2D::Texture2D(int width, int height, const unsigned char* pixels) {
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture2D::~Texture2D() { if (id_) glDeleteTextures(1, &id_); }

Texture2D Texture2D::checkerboard() {
    constexpr std::array<unsigned char, 16> pixels{
        255,255,255,255, 145,170,190,255,
        145,170,190,255, 255,255,255,255
    };
    return Texture2D(2, 2, pixels.data());
}

std::unique_ptr<Texture2D> Texture2D::loadPpm(const std::filesystem::path& path) {
    std::ifstream stream(AssetPath::resolve(path));
    if (!stream) throw std::runtime_error("No se pudo abrir la textura: " + path.string());

    auto readToken = [&stream]() {
        std::string token;
        while (stream >> token) {
            if (!token.empty() && token.front() == '#') {
                std::string ignored;
                std::getline(stream, ignored);
                continue;
            }
            return token;
        }
        throw std::runtime_error("Archivo PPM incompleto");
    };

    if (readToken() != "P3") throw std::runtime_error("Solo se admite PPM ASCII (P3)");
    const int width = std::stoi(readToken());
    const int height = std::stoi(readToken());
    const int maximum = std::stoi(readToken());
    if (width <= 0 || height <= 0 || maximum <= 0) throw std::runtime_error("Cabecera PPM invalida");

    std::vector<unsigned char> pixels(static_cast<std::size_t>(width * height * 4));
    for (int pixel = 0; pixel < width * height; ++pixel) {
        for (int channel = 0; channel < 3; ++channel) {
            const int value = std::stoi(readToken());
            pixels[static_cast<std::size_t>(pixel * 4 + channel)] =
                static_cast<unsigned char>(value * 255 / maximum);
        }
        pixels[static_cast<std::size_t>(pixel * 4 + 3)] = 255;
    }
    return std::make_unique<Texture2D>(width, height, pixels.data());
}

void Texture2D::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, id_);
}

} // namespace Engine
