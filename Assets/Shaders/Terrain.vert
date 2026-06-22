#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aTextureIndex;

uniform mat4 uViewProjection;
uniform mat4 uModel;

out vec3 vNormal;
out vec2 vTexCoord;
flat out float vTextureIndex;

void main() {
    vNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vTexCoord = aTexCoord;
    vTextureIndex = aTextureIndex;
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
}
