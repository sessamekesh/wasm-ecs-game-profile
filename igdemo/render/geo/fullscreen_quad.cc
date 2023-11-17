#include <igdemo/render/geo/fullscreen_quad.h>

namespace {
std::vector<float> kQuadVertices = {
    // positions        // texture Coords
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
};
}

namespace igdemo {

FullscreenQuad::FullscreenQuad(const wgpu::Device& device,
                               const wgpu::Queue& queue)
    : vertexBuffer(create_vec_buffer(device, queue, ::kQuadVertices,
                                     wgpu::BufferUsage::Vertex,
                                     "fullscreen-quad-vb")),
      numVertices(::kQuadVertices.size() / 3) {}

}  // namespace igdemo
