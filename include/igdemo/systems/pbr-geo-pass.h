#ifndef IGDEMO_SYSTEMS_PBR_GEO_PASS_H
#define IGDEMO_SYSTEMS_PBR_GEO_PASS_H

#include <igdemo/render/animated-pbr.h>
#include <igecs/world_view.h>

namespace igdemo {

struct CtxSceneLightingParams {
  pbr::GPULightingParams lightingParams;
  pbr::GPUCameraParams cameraParams;
};

class PbrUploadSceneBuffersSystem {
 public:
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

class PbrUploadPerInstanceBuffersSystem {
 public:
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

class PbrGeoPassSystem {
 public:
  static bool setup_animated(const wgpu::Device& device, igecs::WorldView* wv,
                             const igasset::IgpackDecoder& animated_pbr_decoder,
                             const std::string& animated_pbr_igasset_name);
  static bool setup_static(const wgpu::Device& device, igecs::WorldView* wv,
                           const igasset::IgpackDecoder& static_pbr_decoder,
                           const std::string& static_pbr_igasset_name);

  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
