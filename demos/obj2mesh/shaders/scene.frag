#version 460

layout(set = 1, binding = 0) uniform sampler2D mary_tex;

layout(location = 0) in vec2 frag_texcoords;
layout(location = 1) in vec4 test;

layout(location = 0) out vec4 color;

void main() { color = vec4(texture(mary_tex, frag_texcoords).rgb, 1.0); }