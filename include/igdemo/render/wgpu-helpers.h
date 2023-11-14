#ifndef IGDEMO_RENDER_WGPU_HELPERS_H
#define IGDEMO_RENDER_WGPU_HELPERS_H

#include <webgpu/webgpu_cpp.h>

#include <string>
#include <vector>

namespace igdemo {

struct SizedWgpuBuffer {
  wgpu::Buffer buffer;
  uint32_t size;
};

struct TextureWithMeta {
  wgpu::Texture texture;
  std::uint32_t width;
  std::uint32_t height;
  wgpu::TextureFormat format;
};

wgpu::ShaderModule create_shader_module(const wgpu::Device& device,
                                        const std::string& source,
                                        const char* label);

template <class T>
wgpu::Buffer create_buffer(const wgpu::Device& device, const wgpu::Queue& queue,
                           const T& data, wgpu::BufferUsage usage,
                           const char* label) {
  wgpu::BufferDescriptor buff_desc{};
  buff_desc.label = label;
  buff_desc.size = sizeof(T);
  buff_desc.usage = wgpu::BufferUsage::CopyDst | usage;
  wgpu::Buffer buffer = device.CreateBuffer(&buff_desc);
  queue.WriteBuffer(buffer, 0, &data, sizeof(T));
  return buffer;
}

template <class T>
SizedWgpuBuffer create_vec_buffer(const wgpu::Device& device,
                                  const wgpu::Queue& queue,
                                  const std::vector<T>& data,
                                  wgpu::BufferUsage usage, const char* label) {
  wgpu::BufferDescriptor buff_desc{};
  buff_desc.label = label;
  buff_desc.size = sizeof(T) * data.size();
  buff_desc.usage = wgpu::BufferUsage::CopyDst | usage;
  wgpu::Buffer buffer = device.CreateBuffer(&buff_desc);
  queue.WriteBuffer(buffer, 0, &data[0], sizeof(T) * data.size());
  return {buffer, static_cast<uint32_t>(buff_desc.size)};
}

template <class T>
wgpu::Buffer create_empty_vec_buffer(const wgpu::Device& device,
                                     const wgpu::Queue& queue,
                                     wgpu::BufferUsage usage,
                                     size_t num_elements, const char* label) {
  wgpu::BufferDescriptor buff_desc{};
  buff_desc.label = label;
  buff_desc.size = sizeof(T) * num_elements;
  buff_desc.usage = wgpu::BufferUsage::CopyDst | usage;
  wgpu::Buffer buffer = device.CreateBuffer(&buff_desc);
  return buffer;
}

}  // namespace igdemo

#endif
