#include <igdemo/render/processing/gen-mips.h>

namespace {
const std::uint32_t kWgSize = 8;
}

namespace igdemo {

HdrMipsGenerator HdrMipsGenerator::Create(const wgpu::Device& device,
                                          const IgAsset::WgslSource* wgsl) {
  auto shader_module = create_shader_module(device, wgsl->source()->str(),
                                            "cube-mip-gen-shader");

  wgpu::ComputePipelineDescriptor cpd{};
  cpd.compute.module = shader_module;
  cpd.compute.constantCount = 0;
  cpd.compute.constants = nullptr;
  cpd.compute.entryPoint = wgsl->compute_entry_point()->c_str();
  cpd.label = "cube-mip-gen-pipeline";

  wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&cpd);
  auto bgl = pipeline.GetBindGroupLayout(0);

  return HdrMipsGenerator(pipeline, bgl);
}

void HdrMipsGenerator::gen_cube_mips(const wgpu::Device& device,
                                     const wgpu::Queue& queue,
                                     const wgpu::Texture& cubemap,
                                     std::uint32_t texture_width) const {
  struct MipGenStep {
    std::uint32_t arrayLevel;
    std::uint32_t mipLevel;
    std::uint32_t width;
    wgpu::TextureView inputView;
    wgpu::TextureView outputView;
  };

  std::vector<MipGenStep> mipGenSteps;

  for (std::uint32_t arrayLevel = 0; arrayLevel < 6; arrayLevel++) {
    {
      std::uint32_t prevWidth = texture_width / 2;
      for (std::uint32_t mipLevel = 1; mipLevel < get_num_mips(texture_width);
           mipLevel++) {
        wgpu::TextureViewDescriptor inViewDesc{};
        inViewDesc.baseArrayLayer = arrayLevel;
        inViewDesc.arrayLayerCount = 1;
        inViewDesc.baseMipLevel = mipLevel - 1;
        inViewDesc.mipLevelCount = 1;
        inViewDesc.dimension = wgpu::TextureViewDimension::e2D;
        inViewDesc.format = wgpu::TextureFormat::RGBA16Float;
        inViewDesc.label = "mip-input-view";

        wgpu::TextureViewDescriptor outViewDesc = inViewDesc;
        outViewDesc.baseMipLevel = mipLevel;
        outViewDesc.label = "mip-output-view";

        mipGenSteps.push_back(MipGenStep{arrayLevel, mipLevel, prevWidth,
                                         cubemap.CreateView(&inViewDesc),
                                         cubemap.CreateView(&outViewDesc)});
      }
    }
  }

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
  wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
  pass.SetPipeline(pipeline_);
  for (int i = 0; i < mipGenSteps.size(); i++) {
    const auto& step = mipGenSteps[i];

    wgpu::BindGroupEntry inputEntry{};
    inputEntry.binding = 0;
    inputEntry.textureView = step.inputView;

    wgpu::BindGroupEntry outputEntry{};
    outputEntry.binding = 1;
    outputEntry.textureView = step.outputView;

    wgpu::BindGroupEntry entries[] = {inputEntry, outputEntry};

    wgpu::BindGroupDescriptor bgd{};
    bgd.entries = entries;
    bgd.entryCount = 2;
    bgd.layout = bgl_;
    wgpu::BindGroup bg = device.CreateBindGroup(&bgd);

    std::uint32_t invocationCountX = step.width;
    std::uint32_t invocationCountY = step.width;
    std::uint32_t wgCountX = (invocationCountX + ::kWgSize - 1) / ::kWgSize;
    std::uint32_t wgCountY = (invocationCountX + ::kWgSize - 1) / ::kWgSize;

    pass.SetBindGroup(0, bg);
    pass.DispatchWorkgroups(wgCountX, wgCountY, 1);
  }
  pass.End();

  wgpu::CommandBuffer commands = encoder.Finish();
  queue.Submit(1, &commands);
}

}  // namespace igdemo
