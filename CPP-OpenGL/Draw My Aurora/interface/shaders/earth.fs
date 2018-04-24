#version 330 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D earthMap;

void main() {
    fragColor = texture(earthMap, texCoord);
}
