#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 texCoord;

uniform mat4 model;

void main() {
    vec4 pos = model * vec4(aPos, 1.0);
    gl_Position = vec4(pos.xy, -1.0, 1.0);
    texCoord = aTexCoord;
}
