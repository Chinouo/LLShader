#ifndef SHADER_TYPE_HPP
#define SHADER_TYPE_HPP

#include <vector>
#include <functional>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "3rd/glm/glm/gtc/matrix_transform.hpp"
#include "3rd/glm/glm/gtx/hash.hpp"

namespace LLShader{

typedef struct{
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 light_color;
    alignas(16) glm::mat4 light_mvp;
}PointLight;

typedef struct{
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 light_color;
    alignas(16) glm::mat4 light_mvp;
}DirectionLight;

struct VertexData{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoords;

    bool operator==(const VertexData& other) const{
        return  position == other.position  &&
                normal == other.normal      &&
                texcoords == other.texcoords;
    }
};

struct Mesh{
    std::vector<VertexData> vertices;
    std::vector<u_int32_t> indices;
};

//TODO: make a allocator , imp texture load.



};

// custom hash for remove duplicated vertex
template<> 
struct std::hash<LLShader::VertexData>{
    std::size_t operator()(LLShader::VertexData const& v)const noexcept{
        return  (std::hash<glm::vec3>()(v.position) ^
                (std::hash<glm::vec3>()(v.normal) << 1) >> 1) ^
                (std::hash<glm::vec2>()(v.texcoords) << 1);
    }
};





#endif