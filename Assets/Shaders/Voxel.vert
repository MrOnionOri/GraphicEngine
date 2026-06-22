#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceTransform;

uniform mat4 uViewProjection;

out vec3 vNormal;
out vec2 vTexCoord;

void main() {
    vNormal = normalize(mat3(transpose(inverse(aInstanceTransform))) * aNormal);
    vTexCoord = aTexCoord;
    gl_Position = uViewProjection * aInstanceTransform * vec4(aPosition, 1.0);
}
