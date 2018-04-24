#version 330 core

in vec3 texCoord;

out vec4 fragColor;

uniform samplerCube cubemap;

void main() {
    fragColor = texture(cubemap, texCoord);
}
