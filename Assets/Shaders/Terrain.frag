#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;
flat in float vTextureIndex;
layout (location = 0) out vec4 color;
layout (location = 1) out uint entityId;

uniform sampler2D uTexture;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform uint uEntityId;

void main() {
    vec2 tiledUv = fract(vTexCoord);
    // Sample only texel centers inside a 16x16 tile. This prevents a greedy quad
    // from borrowing pixels from the neighboring atlas tile at repeat boundaries.
    vec2 atlasUv = vec2(
        vTextureIndex / 6.0 + (0.5 + tiledUv.x * 15.0) / 96.0,
        (0.5 + tiledUv.y * 15.0) / 16.0);
    vec3 albedo = texture(uTexture, atlasUv).rgb;
    float diffuse = max(dot(normalize(vNormal), normalize(-uLightDirection)), 0.0);
    color = vec4(albedo * (0.20 + uLightColor * diffuse * uLightIntensity), 1.0);
    entityId = uEntityId;
}
