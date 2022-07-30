#version 460

layout(std140, set = 0, binding = 1) uniform DirectionLight {
  mat4 view_projectio_matrix;
}
dirlight;

layout(location = 0) in vec3 position;

void main() {
  gl_Position = dirlight.view_projectio_matrix * vec4(position, 1.0);
}