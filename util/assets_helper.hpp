#ifndef ASSETS_HELPER_HPP
#define ASSETS_HELPER_HPP

#include <memory>

#include "3rd/tinyobjloader/tiny_obj_loader.h"
#include "demos/common/shader_type.hpp"
#include "log/log.hpp"
#include "util/math_wrap.hpp"

namespace LLShader {

// 加载 obj 文件
// void obj2assets(const std::string& objFilePath,
//                 const std::string& mtlFileSearchPath);

// 只加载obj 文件的顶点
// TODO: perform like std::move with mesh.
void loadObjToMesh(const std::string& file, Mesh& mesh);

// obj to model
// std::shared_ptr<tinyobj::material_t> loadMaterial(const std)

// loadObjWithMtl();

// 把 顶点数据 转换成 vec3
std::vector<glm::vec3> covertVerticesToVector3(const tinyobj::attrib_t& attrib);

// 把 纹理坐标 转换成 vec2
std::vector<glm::vec2> covertTexcoordsToVector2(
    const tinyobj::attrib_t& attrib);

// 把 法线分量 转化成 vec3
std::vector<glm::vec3> covertNormalsToVector3(const tinyobj::attrib_t& attrib);

}  // namespace LLShader

#endif