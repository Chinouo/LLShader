#include "assets_helper.hpp"

#include <unordered_map>

namespace LLShader {

// void obj2assets(const std::string& objFilePath,
//                 const std::string& mtlFileSearchPath) {
//   tinyobj::ObjReader reader;
//   tinyobj::ObjReaderConfig reader_config;
//   reader_config.mtl_search_path = mtlFileSearchPath;
//   if (!reader.ParseFromFile(objFilePath, reader_config)) {
//     if (!reader.Error().empty()) {
//       throw std::runtime_error(reader.Error());
//     }
//   }
//   if (!reader.Warning().empty()) {
//     LogUtil::LogW(reader.Warning());
//   }

//   auto& attrib = reader.GetAttrib();
//   auto& shapes = reader.GetShapes();
//   auto& materials = reader.GetMaterials();

//   // Loop over shapes
//   for (size_t s = 0; s < shapes.size(); s++) {
//     // Loop over faces(polygon)
//     size_t index_offset = 0;
//     for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
//       // 一个面 有几个点
//       size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

//       // Loop over vertices in the face.
//       for (size_t v = 0; v < fv; v++) {
//         // access to vertex
//         tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

//         // v index

//         size_t vertex_idx_offset = 3 * size_t(idx.vertex_index);
//         tinyobj::real_t vx = attrib.vertices[vertex_idx_offset + 0];
//         tinyobj::real_t vy = attrib.vertices[vertex_idx_offset + 1];
//         tinyobj::real_t vz = attrib.vertices[vertex_idx_offset + 2];

//         // Check if `normal_index` is zero or positive. negative = no normal
//         // data
//         if (idx.normal_index >= 0) {
//           size_t normal_idx_offset = 3 * size_t(idx.normal_index);
//           tinyobj::real_t nx = attrib.normals[normal_idx_offset + 0];
//           tinyobj::real_t ny = attrib.normals[normal_idx_offset + 1];
//           tinyobj::real_t nz = attrib.normals[normal_idx_offset + 2];
//         }

//         // Check if `texcoord_index` is zero or positive. negative = no
//         texcoord
//         // data
//         if (idx.texcoord_index >= 0) {
//           size_t tex_idx_offset = 2 * size_t(idx.texcoord_index);
//           tinyobj::real_t tx = attrib.texcoords[tex_idx_offset + 0];
//           tinyobj::real_t ty = attrib.texcoords[tex_idx_offset + 1];
//         }
//       }
//       index_offset += fv;

//       // per-face material
//       shapes[s].mesh.material_ids[f];
//     }
//   }
// }

void loadObjToMesh(const std::string& file, Mesh& mesh) {
  using namespace std;

  tinyobj::ObjReader reader;
  tinyobj::ObjReaderConfig reader_config;
  if (!reader.ParseFromFile(file, reader_config)) {
    if (!reader.Error().empty()) {
      throw std::runtime_error(reader.Error());
    }
  }
  if (!reader.Warning().empty()) {
    LogUtil::LogW(reader.Warning());
  }

  auto& attrib = reader.GetAttrib();
  auto& shapes = reader.GetShapes();

  std::unordered_map<VertexData, u_int32_t> unique_vertices;

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
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
  LogUtil::LogI(file +
                " loded.\n vertices: " + std::to_string(mesh.vertices.size()) +
                "\n indices: " + std::to_string(mesh.indices.size()) + '\n');
}

std::vector<glm::vec3> covertVerticesToVector3(
    const tinyobj::attrib_t& attrib) {
  std::vector<glm::vec3> vs;
  vs.resize(attrib.vertices.size() / 3);
  for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
    glm::vec3 pos = {attrib.vertices[i],       // vx
                     attrib.vertices[i + 1],   // vy
                     attrib.vertices[i + 2]};  // vz
    vs.push_back(pos);
  }

  return vs;
}

std::vector<glm::vec2> covertTexcoordsToVector2(
    const tinyobj::attrib_t& attrib) {
  std::vector<glm::vec2> ts;
  ts.resize(attrib.vertices.size() / 2);
  for (size_t i = 0; i < attrib.vertices.size(); i += 2) {
    glm::vec2 tc = {attrib.vertices[i],       // s
                    attrib.vertices[i + 1]};  // t
    ts.push_back(tc);
  }

  return ts;
}

std::vector<glm::vec3> covertNormalsToVector3(const tinyobj::attrib_t& attrib) {
  std::vector<glm::vec3> ns;
  ns.resize(attrib.normals.size() / 3);
  for (size_t i = 0; i < attrib.normals.size(); i += 3) {
    glm::vec3 normal = {attrib.normals[i],       // nx
                        attrib.normals[i + 1],   // ny
                        attrib.normals[i + 2]};  // nz
    ns[i] = normal;
  }

  return ns;
}

}  // namespace LLShader
