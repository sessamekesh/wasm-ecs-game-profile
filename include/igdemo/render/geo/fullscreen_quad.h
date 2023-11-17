#ifndef IGDEMO_RENDER_FULLSCREEN_QUAD_H
#define IGDEMO_RENDER_FULLSCREEN_QUAD_H

#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

/**
 * @brief WebGPU buffer for a fullscreen quad (used in certain effects)
 */
struct FullscreenQuad {
  FullscreenQuad(const wgpu::Device& device, const wgpu::Queue&);

  SizedWgpuBuffer vertexBuffer;
  std::uint32_t numVertices;
};

}  // namespace igdemo

#endif
