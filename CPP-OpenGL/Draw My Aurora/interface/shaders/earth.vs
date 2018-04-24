#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 texCoord;

layout (std140) uniform Matrices {
    uniform mat4 earthModel;
    uniform mat4 viewProjection;
};

void main() {
    gl_Position = viewProjection * earthModel * vec4(aPos, 1.0);
    texCoord = aTexCoord;
}
