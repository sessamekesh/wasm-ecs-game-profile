#ifndef IGDEMO_SYSTEMS_TONEMAP_PASS_H
#define IGDEMO_SYSTEMS_TONEMAP_PASS_H

#include <igasset/schema/igasset.h>
#include <igecs/world_view.h>
#include <webgpu/webgpu_cpp.h>

namespace igdemo {

class TonemapPass {
 public:
  static bool setup(igecs::WorldView* wv, const wgpu::Device& device,
                    const wgpu::Queue& queue,
                    const IgAsset::WgslSource* aces_tonemap_shader_src,
                    wgpu::TextureFormat output_format);
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
