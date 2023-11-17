#include <igdemo/render/wgpu-helpers.h>

#include <bit>

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

std::uint32_t get_num_mips(std::uint32_t texture_width) {
  return std::bit_width(texture_width);
}

}  // namespace igdemo
