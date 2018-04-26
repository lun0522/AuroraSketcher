#version 330 core

layout (location = 0) in vec3 aPos;

void main() {
    vec3 pos = aPos / 2.0;
    // note the axis of earth is along y direction
    gl_Position = vec4(pos.xz / (pos.y + 0.5), -1.0, 1.0);
}
