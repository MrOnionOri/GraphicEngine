#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 4) in mat4 aInstanceTransform;

uniform mat4 uViewProjection;

void main() {
    gl_Position = uViewProjection * aInstanceTransform * vec4(aPosition, 1.0);
}
