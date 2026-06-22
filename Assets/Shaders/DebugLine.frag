#version 330 core

layout (location = 0) out vec4 color;
layout (location = 1) out uint entityId;

uniform vec3 uColor;

void main() {
    color = vec4(uColor, 1.0);
    entityId = 0u;
}
