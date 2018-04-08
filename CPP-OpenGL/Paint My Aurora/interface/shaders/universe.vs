#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 texCoord;

uniform mat4 model;
uniform mat4 viewProjection;

void main() {
    gl_Position = viewProjection * model * vec4(aPos, 1.0);
    gl_Position.zw = vec2(1.0); // depth as large as possible
    texCoord = aPos;
}
