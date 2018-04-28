#version 330 core

layout (location = 0) in vec2 aPos;

out vec3 fragPos;

uniform vec3 origin;
uniform vec3 xAxis;
uniform vec3 yAxis;

void main() {
    gl_Position = vec4(aPos, -1.0, 1.0);
    fragPos = origin + aPos.x * xAxis + aPos.y * yAxis;
}
