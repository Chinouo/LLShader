#ifndef MATH_WRAP_HPP
#define MATH_WRAP_HPP

// #define GLM_FORCE_MESSAGES
#define GLM_FORCE_CXX17
#define GLM_FORCE_PURE
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_SSE2
#include "3rd/glm/glm/glm.hpp"
#include "3rd/glm/glm/gtc/matrix_transform.hpp"
#include "3rd/glm/glm/gtc/quaternion.hpp"

namespace LLShader {
constexpr glm::quat quat_identity{1.f, 0.f, 0.f, 0.f};

}

#endif