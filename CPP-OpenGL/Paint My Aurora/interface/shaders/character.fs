#version 330 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D glyph;

void main() {
    fragColor = texture(glyph, texCoord);
}
