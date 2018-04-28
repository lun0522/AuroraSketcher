#version 330 core

layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

uniform float lineWidth;

void main() {
    vec2 point0 = vec2(gl_in[0].gl_Position);
    vec2 point1 = vec2(gl_in[1].gl_Position);
    vec2 tangent = normalize(point1 - point0);
    vec2 normal = vec2(-tangent.y, tangent.x);
    vec2 bias = normal * lineWidth * 0.5;
    
    gl_Position = vec4(point0 - bias, -1.0, 1.0);
    EmitVertex();
    gl_Position = vec4(point1 - bias, -1.0, 1.0);
    EmitVertex();
    gl_Position = vec4(point0 + bias, -1.0, 1.0);
    EmitVertex();
    gl_Position = vec4(point1 + bias, -1.0, 1.0);
    EmitVertex();
    EndPrimitive();
}
