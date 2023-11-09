#ifndef IGASSET_VERTEX_TYPES_H
#define IGASSET_VERTEX_TYPES_H

#include <glm/glm.hpp>

namespace igasset {

enum class IndexBufferType {
  Uint16,
  Uint32,
};

struct PosNormalVertexData3D {
  glm::vec3 Position;
  glm::vec3 Normal;
};

struct TangentBitangentVertexData3D {
  glm::vec3 Tangent;
  glm::vec3 Bitangent;
};

struct TexcoordVertexData {
  glm::vec2 Texcoord;
};

struct BoneWeightsVertexData {
  glm::vec4 Weights;
  glm::u8vec4 Indices;
};

}  // namespace igasset

#endif
