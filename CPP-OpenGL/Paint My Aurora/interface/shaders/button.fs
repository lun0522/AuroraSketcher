#version 330 core

in vec2 texCoord;

out vec4 fragColor;

uniform float alpha;
uniform vec3 color;
uniform sampler2D alphaMap;

void main() {
    fragColor = vec4(color, alpha * texture(alphaMap, texCoord).r);
}
