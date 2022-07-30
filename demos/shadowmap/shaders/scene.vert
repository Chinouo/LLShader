#version 460

layout(set = 0, binding = 0) uniform CameraData { mat4 camera_view_proj; }
camera_data;

layout(std140, set = 0, binding = 1) uniform DirectionLight {
  mat4 view_projectio_matrix;
}
dirlight;
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoords;

layout(location = 0) out vec3 frag_world_position;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_texcoords;
layout(location = 3) out vec4 light_space_position;

void main() {
  gl_Position = camera_data.camera_view_proj * vec4(position, 1.0);
  frag_world_position = position;
  frag_normal = normal;
  frag_texcoords = texcoords;
  light_space_position = dirlight.view_projectio_matrix * vec4(position, 1.0);
}