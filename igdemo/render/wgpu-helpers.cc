#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

wgpu::ShaderModule create_shader_module(const wgpu::Device& device,
                                        const std::string& source,
                                        const char* label) {
  wgpu::ShaderModuleWGSLDescriptor wgsl_desc{};
  wgsl_desc.code = &source[0];

  wgpu::ShaderModuleDescriptor desc{};
  desc.nextInChain = &wgsl_desc;
  desc.label = label;

  return device.CreateShaderModule(&desc);
}

}  // namespace igdemo
