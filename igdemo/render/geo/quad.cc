#include <igdemo/render/geo/quad.h>

namespace igdemo {

std::vector<igasset::PosNormalVertexData3D> Quad::get_vertices() const {
  return {{{-width / 2.f, 0.f, -depth / 2.f}, {0.f, 1.f, 0.f}},
          {{width / 2.f, 0.f, -depth / 2.f}, {0.f, 1.f, 0.f}},
          {{-width / 2.f, 0.f, depth / 2.f}, {0.f, 1.f, 0.f}},
          {{width / 2.f, 0.f, depth / 2.f}, {0.f, 1.f, 0.f}}};
}
std::vector<std::uint16_t> Quad::get_indices() const {
  return {0, 2, 1, 1, 2, 3};
}

}  // namespace igdemo
