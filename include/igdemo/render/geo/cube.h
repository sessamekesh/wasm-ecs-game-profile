#ifndef IGDEMO_RENDER_GEO_CUBE_H
#define IGDEMO_RENDER_GEO_CUBE_H

#include <igdemo/render/wgpu-helpers.h>
#include <webgpu/webgpu_cpp.h>

namespace igdemo {

/**
 * @brief WebGPU buffer for a unit cube (positions only) used in cubemap
 * rendering
 */
struct CubemapUnitCube {
  CubemapUnitCube(const wgpu::Device&, const wgpu::Queue&);

  SizedWgpuBuffer vertexBuffer;
  std::uint32_t numVertices;
};

}  // namespace igdemo

#endif
