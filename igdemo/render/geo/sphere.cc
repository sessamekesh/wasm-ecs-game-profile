#include <igdemo/render/geo/sphere.h>

#include <glm/gtc/constants.hpp>

namespace igdemo {

// Adapted from
//  https://github.com/JoeyDeVries/LearnOpenGL/blob/3e94252892660902bef62068c35253cbe3464c9b/src/6.pbr/2.1.2.ibl_irradiance/ibl_irradiance.cpp#L406
//  because I'm lazy
std::vector<igasset::PosNormalVertexData3D> SphereGenerator::get_vertices()
    const {
  std::vector<igasset::PosNormalVertexData3D> verts;

  verts.reserve(static_cast<std::uint32_t>(xSegments) *
                static_cast<std::uint32_t>(ySegments));

  for (std::uint16_t x = 0u; x < xSegments; x++) {
    for (std::uint16_t y = 0u; y < ySegments; y++) {
      float xSegment = static_cast<float>(x) / static_cast<float>(xSegments);
      float ySegment = static_cast<float>(y) / static_cast<float>(ySegments);

      igasset::PosNormalVertexData3D vert{};
      vert.Position.x = (glm::cos(xSegment * glm::two_pi<float>()) *
                         glm::sin(ySegment * glm::pi<float>())) *
                        radius;
      vert.Position.y =
          glm::cos(ySegment * glm::pi<float>()) * radius + yOffset;
      vert.Position.z = (glm::sin(xSegment * glm::two_pi<float>()) *
                         glm::sin(ySegment * glm::pi<float>())) *
                        radius;

      vert.Normal = vert.Position;
      verts.push_back(vert);
    }
  }

  return verts;
}

std::vector<std::uint16_t> SphereGenerator::get_indices() const {
  std::vector<std::uint16_t> indices;
  indices.reserve(xSegments * ySegments * 2u);

  for (std::uint16_t y = 0u; y <= ySegments; y++) {
    for (std::uint16_t x = 0u; x <= xSegments; x++) {
      // Two triangles - [TL,BL,TR], [TR,BL,BR]
      std::uint16_t TL = y * (xSegments + 1) + x;
      std::uint16_t TR = y * (xSegments + 1) + ((x + 1) % xSegments);
      std::uint16_t BL = ((y + 1) % ySegments) * (xSegments + 1) + x;
      std::uint16_t BR =
          ((y + 1) % ySegments) * (xSegments + 1) + ((x + 1) % xSegments);

      indices.push_back(TL);
      indices.push_back(BL);
      indices.push_back(TR);
      indices.push_back(TR);
      indices.push_back(BL);
      indices.push_back(BR);
    }
  }

  return indices;
}

}  // namespace igdemo
