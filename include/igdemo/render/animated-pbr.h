#ifndef IGDEMO_RENDER_ANIMATED_PBR_H
#define IGDEMO_RENDER_ANIMATED_PBR_H

#include <igasset/igpack_decoder.h>
#include <igecs/world_view.h>
#include <webgpu/webgpu_cpp.h>

#include <optional>

namespace igdemo {

struct CtxAnimatedPbrPipeline {
 public:
  //
  // GPU UBO types
  //
  struct GPUCameraParams {
    glm::mat4 matViewProj;
    glm::vec3 cameraPos;
    float __padding_1__ = 0.f;
  };

  struct GPULightingParams {
    glm::vec3 sunColor;
    float ambientCoefficient;
    glm::vec3 sunDirection;
    float __padding_1__ = 0.f;
  };

  struct GPUPbrColorParams {
    glm::vec3 albedo;
    float metallic;
    float roughness;
    float __padding__[3] = {0.f, 0.f, 0.f};
  };

  //
  // Context component data
  //
  wgpu::RenderPipeline pipeline;
  wgpu::BindGroupLayout frame_bgl;
  wgpu::BindGroupLayout obj_bgl;
  wgpu::BindGroupLayout skin_bgl;
  wgpu::BindGroupLayout ibl_bgl;

  //
  // Construction
  //
  static std::optional<CtxAnimatedPbrPipeline> Create(
      const igasset::IgpackDecoder& decoder,
      const std::string& wgsl_igasset_name, const wgpu::Device& device);

  CtxAnimatedPbrPipeline(wgpu::RenderPipeline rp, wgpu::BindGroupLayout fbgl,
                         wgpu::BindGroupLayout obgl, wgpu::BindGroupLayout sbgl,
                         wgpu::BindGroupLayout iblbgl)
      : pipeline(rp),
        frame_bgl(fbgl),
        obj_bgl(obgl),
        skin_bgl(sbgl),
        ibl_bgl(iblbgl) {}

  // Rule o' 5
  CtxAnimatedPbrPipeline(const CtxAnimatedPbrPipeline&) = delete;
  CtxAnimatedPbrPipeline& operator=(const CtxAnimatedPbrPipeline&) = delete;

  CtxAnimatedPbrPipeline(CtxAnimatedPbrPipeline&&) noexcept = default;
  CtxAnimatedPbrPipeline& operator=(CtxAnimatedPbrPipeline&&) noexcept =
      default;
};

struct AnimatedPbrFrameBindGroup {
  wgpu::BindGroup frameBindGroup;
  wgpu::Buffer cameraParams;
  wgpu::Buffer lightingParams;

  AnimatedPbrFrameBindGroup(const wgpu::Device& device,
                            const wgpu::BindGroupLayout& frame_bgl,
                            const wgpu::Buffer& cameraParamsBuffer,
                            const wgpu::Buffer& lightingParamsBuffer);
};

struct AnimatedPbrIblBindGroup {
  wgpu::BindGroup bindGroup;

  wgpu::Sampler iblSampler;
  wgpu::TextureView irradianceMapView;
  wgpu::TextureView prefilteredEnvView;
  wgpu::TextureView brdfLutView;

  AnimatedPbrIblBindGroup(const wgpu::Device& device,
                          const wgpu::BindGroupLayout& ibl_bgl,
                          const wgpu::Texture& irradianceMap,
                          const wgpu::Texture& prefilteredEnvMap,
                          const wgpu::Texture& brdfLut);
};

struct AnimatedPbrMaterial {
  wgpu::BindGroup objBindGroup;
  wgpu::Buffer materialBuffer;

  AnimatedPbrMaterial(
      const wgpu::Device& device, const wgpu::Queue& queue,
      const wgpu::BindGroupLayout& obj_bgl,
      const CtxAnimatedPbrPipeline::GPUPbrColorParams& material);
};

struct AnimatedPbrGeometry {
  wgpu::Buffer vertexBuffer;
  std::uint32_t vertexBufferSize;

  wgpu::Buffer boneWeightsBuffer;
  std::uint32_t boneWeightsBufferSize;

  wgpu::Buffer indexBuffer;
  std::uint32_t indexBufferSize;
  std::uint32_t numIndices;
  wgpu::IndexFormat indexFormat;

  std::vector<std::string> boneNames;
  std::vector<glm::mat4> invBindPoses;

  AnimatedPbrGeometry(
      const wgpu::Device& device, const wgpu::Queue& queue,
      const std::vector<igasset::PosNormalVertexData3D>& pos_norm_data,
      const std::vector<igasset::BoneWeightsVertexData>& bone_weights,
      const std::vector<std::uint16_t>& indices,
      std::vector<std::string> boneNames, std::vector<glm::mat4> invBindPoses);
};

struct AnimatedPbrSkinBindGroup {
  wgpu::BindGroup skinBindGroup;
  wgpu::Buffer skinMatrixBuffer;
  wgpu::Buffer worldTransformBuffer;

  AnimatedPbrSkinBindGroup(const wgpu::Device& device, const wgpu::Queue& queue,
                           const wgpu::BindGroupLayout& skin_bgl,
                           std::uint32_t num_bones);
  void update(const wgpu::Queue& queue, const std::vector<glm::mat4>& skin,
              const glm::mat4& worldTransform) const;
};

struct AnimatedPbrInstance {
  const AnimatedPbrMaterial* material;
  const AnimatedPbrGeometry* geometry;
};

}  // namespace igdemo

#endif
