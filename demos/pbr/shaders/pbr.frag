#version 460

const float PI = 3.14159265359;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 ViewProjMatrix;
  vec3 position;
}
camera;

layout(set = 0, binding = 1) uniform PointLight {
  vec3 position;
  vec3 color;
}
point_light;

layout(set = 0, binding = 2) uniform Material {
  float roughness;
  float metalic;
}
material;

layout(location = 0) in vec3 frag_world_position;
layout(location = 1) in vec3 frag_normal;

layout(location = 0) out vec3 color;

vec3 F_Schlick(vec3 F0, vec3 N, vec3 V) {
  float cos_theta = dot(N, V);
  return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

float D_GGX(vec3 N, vec3 H, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float dotNH = clamp(dot(N, H), 0.0, 1.0);
  float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
  return alpha2 / (PI * denom * denom);
}

float G_SchlicksmithGGX(vec3 N, vec3 V, vec3 L, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float dotNL = clamp(dot(N, L), 0.0, 1.0);
  float dotNV = clamp(dot(N, V), 0.0, 1.0);

  float k2 = 1.0 - k;

  float GL = dotNL / ((dotNL * k2) + k);
  float GV = dotNL / ((dotNV * k2) + k);

  return GL * GV;
}

void main() {
  vec3 N = normalize(frag_normal);
  vec3 V = normalize(camera.position - frag_world_position);
  vec3 L = normalize(point_light.position - frag_world_position);
  vec3 H = normalize(V + L);

  color = vec3(0.0, 0.0, 0.0);

  float dotNL = dot(N, L);
  if (dotNL > 0) {
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float D = D_GGX(N, H, material.roughness);
    vec3 F0 = mix(vec3(0.04), vec3(0.7), material.metalic);
    vec3 F = F_Schlick(F0, N, V);
    float G = G_SchlicksmithGGX(N, V, L, material.roughness);
    color += ((D * F * G) / (4.0 * dotNV * dotNL)) * point_light.color;
  }
}