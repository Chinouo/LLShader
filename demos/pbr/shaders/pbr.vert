#version 460

layout(set = 0, binding = 0) uniform CameraData { mat4 ViewProjMatrix; }
camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoords;

layout(location = 0) out vec3 frag_world_position;
layout(location = 1) out vec3 frag_normal;

void main() {
  // model is identity.
  gl_Position = camera.ViewProjMatrix * vec4(position, 1.0);
  frag_world_position = position;
  frag_normal = normal;
}