#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

uniform float depth;

void main() {
    gl_Position = vec4(aPos, depth, 1.0);
    texCoord = aTexCoord;
}
