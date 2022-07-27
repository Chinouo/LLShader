#version 460

layout(set = 0, binding = 0) uniform CameraUniformObject {
  mat4 model;
  mat4 view;
  mat4 proj;
}
ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoords;

layout(location = 0) out vec2 frag_texcoords;
layout(location = 1) out vec4 test;

void main() {
  gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
  test = ubo.proj * ubo.view * vec4(1.f, 1.f, 0.f, 1.f);
  frag_texcoords = texcoords;
}