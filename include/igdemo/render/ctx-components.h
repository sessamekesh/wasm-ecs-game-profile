#ifndef IGDEMO_RENDER_CTX_COMPONENTS_H
#define IGDEMO_RENDER_CTX_COMPONENTS_H

#include <webgpu/webgpu_cpp.h>

namespace igdemo {

struct CtxWgpuDevice {
  wgpu::Device device;
  wgpu::Queue queue;
  wgpu::TextureFormat renderTargetFormat;
  wgpu::TextureView renderTarget;
};

}  // namespace igdemo

#endif
