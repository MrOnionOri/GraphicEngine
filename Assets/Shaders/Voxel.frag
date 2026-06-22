#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;
layout (location = 0) out vec4 color;
layout (location = 1) out uint entityId;

uniform vec3 uBaseColor;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform sampler2D uTexture;
uniform uint uEntityId;

void main() {
    vec3 albedo = texture(uTexture, vTexCoord).rgb * uBaseColor;
    float diffuse = max(dot(normalize(vNormal), normalize(-uLightDirection)), 0.0);
    vec3 ambient = albedo * 0.16;
    vec3 lit = albedo * uLightColor * diffuse * uLightIntensity;
    color = vec4(ambient + lit, 1.0);
    entityId = uEntityId;
}
