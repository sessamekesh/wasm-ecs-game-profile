#ifndef IGDEMO_RENDER_GEO_SPHERE_H
#define IGDEMO_RENDER_GEO_SPHERE_H

#include <igasset/vertex_types.h>

#include <vector>

namespace igdemo {

struct SphereGenerator {
  std::uint16_t xSegments;
  std::uint16_t ySegments;

  std::vector<igasset::PosNormalVertexData3D> get_vertices() const;
  std::vector<std::uint16_t> get_indices() const;
};

}  // namespace igdemo

#endif
