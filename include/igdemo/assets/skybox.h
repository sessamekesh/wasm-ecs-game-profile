#ifndef IGDEMO_ASSETS_SKYBOX_H
#define IGDEMO_ASSETS_SKYBOX_H

#include <igasync/promise.h>
#include <igdemo/igdemo-app.h>
#include <igdemo/render/animated-pbr.h>
#include <igdemo/render/processing/equirect-to-cubemap.h>
#include <igdemo/render/processing/gen-mips.h>
#include <igdemo/render/static-pbr.h>

namespace igdemo {

struct CtxMipGen {
  HdrMipsGenerator hdrMipGenerator;
};

struct CtxHdrSkybox {
  wgpu::Texture cubemap;
  wgpu::TextureView cubemapView;

  wgpu::Texture irradianceMap;
  wgpu::TextureView irradianceMapView;

  wgpu::Texture prefilterMap;
  wgpu::TextureView prefilterMapView;

  wgpu::Texture brdfLut;
  wgpu::TextureView brdfLutView;

  AnimatedPbrIblBindGroup iblBindGroupAnimated;
  StaticPbrIblBindGroup iblBindGroupStatic;

  // TODO (sessamekesh): Need the following generated things as well:
  // prefilter map
  // See:
  // https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.2.2.ibl_specular_textured/ibl_specular_textured.cpp
};

std::shared_ptr<igasync::Promise<std::vector<std::string>>> load_skybox(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::Promise<bool>> pbr_shader_setup_promise,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks);

}  // namespace igdemo

#endif
