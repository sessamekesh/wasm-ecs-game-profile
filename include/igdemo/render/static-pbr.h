#ifndef IGDEMO_RENDER_STATIC_PBR_H
#define IGDEMO_RENDER_STATIC_PBR_H

#include <igasset/igpack_decoder.h>
#include <igdemo/render/pbr-common.h>
#include <igecs/world_view.h>
#include <webgpu/webgpu_cpp.h>

#include <optional>

namespace igdemo {

struct CtxStaticPbrPipeline {
 public:
  //
  // Context component data
  //
  wgpu::RenderPipeline pipeline;
  wgpu::BindGroupLayout frame_bgl;
  wgpu::BindGroupLayout obj_bgl;
  wgpu::BindGroupLayout model_bgl;
  wgpu::BindGroupLayout ibl_bgl;

  //
  // Construction
  //
  static std::optional<CtxStaticPbrPipeline> Create(
      const igasset::IgpackDecoder& decoder,
      const std::string& wgsl_igasset_name, const wgpu::Device& device);

  CtxStaticPbrPipeline(wgpu::RenderPipeline p, wgpu::BindGroupLayout bgl0,
                       wgpu::BindGroupLayout bgl1, wgpu::BindGroupLayout bgl2,
                       wgpu::BindGroupLayout bgl3)
      : pipeline(p),
        frame_bgl(bgl0),
        obj_bgl(bgl1),
        model_bgl(bgl2),
        ibl_bgl(bgl3) {}

  // Rule o' 5
  CtxStaticPbrPipeline() = delete;
  CtxStaticPbrPipeline(const CtxStaticPbrPipeline&) = delete;
  CtxStaticPbrPipeline& operator=(const CtxStaticPbrPipeline&) = delete;

  CtxStaticPbrPipeline(CtxStaticPbrPipeline&&) noexcept = default;
  CtxStaticPbrPipeline& operator=(CtxStaticPbrPipeline&&) noexcept = default;
};

struct StaticPbrFrameBindGroup {
  wgpu::BindGroup frameBindGroup;
  wgpu::Buffer cameraParams;
  wgpu::Buffer lightingParams;

  StaticPbrFrameBindGroup(const wgpu::Device& device,
                          const wgpu::BindGroupLayout& frame_bgl,
                          const wgpu::Buffer& cameraParamsBuffer,
                          const wgpu::Buffer& lightingParamsBuffer);
};

struct StaticPbrModelBindGroup {
  wgpu::BindGroup bindGroup;

  wgpu::Buffer worldTransformBuffer;

  StaticPbrModelBindGroup(const wgpu::Device& device, const wgpu::Queue& queue,
                          const wgpu::BindGroupLayout& model_bgl);

  void update(const wgpu::Queue& queue, const glm::mat4& worldTransform) const;
};

struct StaticPbrIblBindGroup {
  wgpu::BindGroup bindGroup;

  wgpu::Sampler iblSampler;
  wgpu::TextureView irradianceMapView;
  wgpu::TextureView prefilteredEnvView;
  wgpu::TextureView brdfLutView;

  StaticPbrIblBindGroup(const wgpu::Device& device,
                        const wgpu::BindGroupLayout& ibl_bgl,
                        const wgpu::Texture& irradianceMap,
                        const wgpu::Texture& prefilteredEnvMap,
                        const wgpu::Texture& brdfLut);
};

struct StaticPbrMaterial {
  wgpu::BindGroup objBindGroup;
  wgpu::Buffer materialBuffer;

  StaticPbrMaterial(const wgpu::Device& device, const wgpu::Queue& queue,
                    const wgpu::BindGroupLayout& obj_bgl,
                    const pbr::GPUPbrColorParams& material);
};

struct StaticPbrGeometry {
  wgpu::Buffer vertexBuffer;
  std::uint32_t vertexBufferSize;

  wgpu::Buffer indexBuffer;
  std::uint32_t indexBufferSize;
  std::uint32_t numIndices;
  wgpu::IndexFormat indexFormat;

  StaticPbrGeometry(
      const wgpu::Device& device, const wgpu::Queue& queue,
      const std::vector<igasset::PosNormalVertexData3D>& pos_norm_data,
      const std::vector<std::uint16_t>& indices);
};

struct StaticPbrInstance {
  const StaticPbrMaterial* material;
  const StaticPbrGeometry* geometry;
};

}  // namespace igdemo

#endif
