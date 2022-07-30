#version 460

layout(set = 1, binding = 0) uniform sampler2D mary_texture;

layout(set = 1, binding = 1) uniform sampler2D shadowmap;

layout(location = 0) in vec3 frag_world_position;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_texcoords;
layout(location = 3) in vec4 light_space_position;

layout(location = 0) out vec3 color;

void easy_shadowmap() {}

vec3 simple_shadow(vec3 pos) {
  vec3 shadow;
  if (pos.z - 0.0001 > texture(shadowmap, pos.st).r) {
    shadow = texture(mary_texture, frag_texcoords).rgb * 0.1;
  } else {
    shadow = texture(mary_texture, frag_texcoords).rgb;
  }
  return shadow;
}

vec3 pcf_shadow(vec3 pos) {
  ivec2 texDim = textureSize(shadowmap, 0);
  float scale = 1.5;
  float dx = scale * 1.0 / float(texDim.x);
  float dy = scale * 1.0 / float(texDim.y);
  float shadowFactor = 0.0;
  int range = 1;

  for (int x = -range; x <= range; x++) {
    for (int y = -range; y <= range; y++) {
      float pcf = texture(shadowmap, pos.st + vec2(x * dx, y * dy)).r;
      if (pos.z - 0.0001 > pcf) {
        shadowFactor += 1.0;
      }
    }
  }
  shadowFactor /= 9.0;

  return texture(mary_texture, frag_texcoords).rgb * 0.7 * (1.0 - shadowFactor);
}

void main() {
  vec3 pos = light_space_position.xyz / light_space_position.w;
  pos = pos * 0.5 + 0.5;
  pos.z = light_space_position.z;  // find this cost many time...

  color = pcf_shadow(pos);

  // color = vec3(shadowFactor);
}