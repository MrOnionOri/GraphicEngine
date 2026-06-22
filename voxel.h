#pragma once

#include <GL/glew.h>
#include <vector>

class Voxel {
public:
    // Constructor
    Voxel(float size = 1.0f) {
        initVoxel(size);

        // Configurar el VAO y el VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO); // Para los índices del cubo

        // Vincular VAO
        glBindVertexArray(VAO);

        // Vincular y llenar el VBO con los datos de los vértices
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Vincular y llenar el EBO con los datos de los índices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Especificar el layout de los datos de los vértices
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Desvincular VAO y VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Destructor
    ~Voxel() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    Voxel(const Voxel&) = delete;
    Voxel& operator=(const Voxel&) = delete;

    // Método para dibujar el vóxel
    void draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

private:
    std::vector<float> vertices;      // Vértices del cubo (vóxel)
    std::vector<unsigned int> indices; // Índices para los triángulos del cubo
    GLuint VAO, VBO, EBO;              // OpenGL VAO, VBO, y EBO (Element Buffer Object)

    // Inicializar un vóxel (cubo)
    void initVoxel(float size) {
        float halfSize = size / 2.0f;

        // Definir los vértices del cubo (vóxel) en 3D
        vertices = {
            // Frente
            -halfSize, -halfSize,  halfSize,  // Vértice 0
             halfSize, -halfSize,  halfSize,  // Vértice 1
             halfSize,  halfSize,  halfSize,  // Vértice 2
            -halfSize,  halfSize,  halfSize,  // Vértice 3
            // Atrás
            -halfSize, -halfSize, -halfSize,  // Vértice 4
             halfSize, -halfSize, -halfSize,  // Vértice 5
             halfSize,  halfSize, -halfSize,  // Vértice 6
            -halfSize,  halfSize, -halfSize   // Vértice 7
        };

        indices = {
            // Cara frontal
            0, 1, 2,
            2, 3, 0,
            4, 5, 6,
            6, 7, 4,
            // Cara izquierda
            0, 3, 7,
            7, 4, 0,
            // Cara derecha
            1, 5, 6,
            6, 2, 1,
            // Cara inferior
            0, 1, 5,
            5, 4, 0,
            // Cara superior
            3, 2, 6,
            6, 7, 3
        };
    }
};
