#ifndef MODEL_PROTOTYPE_HPP
#define MODEL_PROTOTYPE_HPP

#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>

#include "3rd/tinyobjloader/tiny_obj_loader.h"
#include "demos/common/shader_type.hpp"
#include "engine/matrix.hpp"
#include "log/log.hpp"
#include "render/render_manager.hpp"

namespace LLShader {

typedef struct {
  VkBuffer vert_buffer;
  VkDeviceMemory vert_memory;
  VkBuffer idx_buffer;
  VkDeviceMemory idx_memory;
} ProtoTypeGPUData;

/// TODO: using a big material lib, mesh lib, using flight weight or prototype
/// pattern to implement this class.
/// Load Obj file only
class ModelProtoType {
 public:
  // mtl file auto load in same dictionary.
  ModelProtoType(const std::string& obj_file) {
    using namespace tinyobj;
    ObjReader reader;
    ObjReaderConfig reader_config;
    reader_config.triangulate = true;

    if (!reader.ParseFromFile(obj_file, reader_config)) {
      if (!reader.Error().empty()) {
        throw std::runtime_error(reader.Error());
      }
    }
    if (!reader.Warning().empty()) {
      LogUtil::LogW(reader.Warning());
    }

    // load material
    materials = reader.GetMaterials();
    // load vertex data
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    std::unordered_map<VertexData, u_int32_t> unique_vertices;

    sub_meshes.resize(shapes.size());
    for (size_t i = 0; i < sub_meshes.size(); ++i) {
      auto& shape = shapes[i];
      auto& mesh = sub_meshes[i];
      mesh.name = shape.name;
      // TODO: performance may be optimized here
      for (size_t j = 0; j < shape.mesh.indices.size(); ++j) {
        auto& index = shape.mesh.indices[j];
        VertexData vertex;
        // 提取 position
        size_t v_start = index.vertex_index * 3;
        auto vx = attrib.vertices[v_start];
        auto vy = attrib.vertices[v_start + 1];
        auto vz = attrib.vertices[v_start + 2];
        vertex.position = {vx, vy, vz};

        // 提取 normal
        if (index.normal_index >= 0) {
          size_t n_start = index.normal_index * 3;
          auto nx = attrib.normals[n_start];
          auto ny = attrib.normals[n_start + 1];
          auto nz = attrib.normals[n_start + 2];
          vertex.normal = {nx, ny, nz};
        }

        // 提取 texcoords
        // reverse y for vulkan
        if (index.texcoord_index >= 0) {
          size_t t_start = 2 * index.texcoord_index;
          auto s = attrib.texcoords[t_start];
          auto t = 1.0 - attrib.texcoords[t_start + 1];
          vertex.texcoords = {s, t};
        }
        // not exitst
        if (!unique_vertices.count(vertex)) {
          unique_vertices[vertex] = mesh.vertices.size();
          mesh.vertices.push_back(vertex);
        }
        mesh.indices.push_back(unique_vertices[vertex]);
      }
    }

    LogUtil::LogI(obj_file + " loded.\n");

    for (auto& mesh : sub_meshes) {
      LogUtil::LogI("mesh " + mesh.name +
                    "\n vertices:" + std::to_string(mesh.vertices.size()) +
                    "\n indices: " + std::to_string(mesh.indices.size()) +
                    '\n');
    }
  }

  ModelProtoType(const ModelProtoType&) = delete;

  inline const std::vector<Mesh>& getMeshes() const { return sub_meshes; }

  inline std::vector<ProtoTypeGPUData> generateGPUData() const {
    assert(global_matrix_engine.render_manager.use_count());
    auto& manager = global_matrix_engine.render_manager;
    std::vector<ProtoTypeGPUData> data(sub_meshes.size());

    for (size_t i = 0; i < data.size(); ++i) {
      manager->createDeviceOnlyBuffer(data[i].idx_buffer, data[i].idx_memory,
                                      sub_meshes[i].getIndicesByteSize(),
                                      (void*)sub_meshes[i].indices.data(),
                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      manager->createDeviceOnlyBuffer(data[i].vert_buffer, data[i].vert_memory,
                                      sub_meshes[i].getVerticesByteSize(),
                                      (void*)sub_meshes[i].vertices.data(),
                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    return data;
  }

 private:
  // if have multi mode, there is duplication data, also vertex number may
  // overflow.
  std::vector<Mesh> sub_meshes;

  // tinyobj material is better than me.
  std::vector<tinyobj::material_t> materials;
};

}  // namespace LLShader

#endif