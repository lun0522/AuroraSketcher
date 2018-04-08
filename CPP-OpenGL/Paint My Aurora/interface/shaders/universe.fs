#version 330 core

in vec3 texCoord;

out vec4 fragColor;

uniform samplerCube cubemap0;

void main() {
    fragColor = texture(cubemap0, texCoord);
}
