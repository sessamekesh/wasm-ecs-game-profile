#ifndef IGDEMO_RENDER_GEO_QUAD_H
#define IGDEMO_RENDER_GEO_QUAD_H

#include <igasset/vertex_types.h>

#include <vector>

namespace igdemo {

struct Quad {
  float width;
  float depth;

  std::vector<igasset::PosNormalVertexData3D> get_vertices() const;
  std::vector<std::uint16_t> get_indices() const;
};

}  // namespace igdemo

#endif
