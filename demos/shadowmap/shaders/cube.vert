#version 460

layout(set = 0, binding = 0) uniform CameraData { mat4 view_projectio_matrix; }
camera;

layout(set = 1, binding = 0) readonly buffer instance_data {
  mat4 model[];
  vec3 color[];
};

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 out_color;

void main() {
  gl_Position = camera.view_projectio_matrix *
                instance_data.model[gl_InstanceIndex] * vec4(position, 1.0);

  out_color = instance_data.color[gl_InstanceIndex];
}