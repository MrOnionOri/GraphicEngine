#include <GL/glew.h>
#include <vector>

class GeometricShape {
public:
    enum ShapeType {
        TRIANGLE,
        SQUARE
    };

    // Constructor que toma el tipo de figura
    GeometricShape(ShapeType type) {
        if (type == TRIANGLE) {
            initTriangle();
        }
        else if (type == SQUARE) {
            initSquare();
        }

        // Configurar el VAO y el VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Vincular VAO
        glBindVertexArray(VAO);

        // Vincular y llenar el VBO con los datos de los vértices
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Especificar el layout de los datos de los vértices
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Desvincular el VAO y el VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Destructor para liberar recursos
    ~GeometricShape() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    GeometricShape(const GeometricShape&) = delete;
    GeometricShape& operator=(const GeometricShape&) = delete;

    // Método para dibujar la figura
    void draw() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 3));
        glBindVertexArray(0);
    }

private:
    std::vector<float> vertices;  // Vértices de la figura
    GLuint VAO, VBO;              // OpenGL VAO y VBO

    // Inicializar un triángulo
    void initTriangle() {
        vertices = {
            // Posiciones de los vértices del triángulo
             0.0f,  0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f
        };
    }

    // Inicializar un cuadrado (dos triángulos)
    void initSquare() {
        vertices = {
            // Primer triángulo
             0.5f,  0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
            // Segundo triángulo
             0.5f,  0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f
        };
    }
};
