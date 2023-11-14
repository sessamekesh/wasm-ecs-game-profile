#ifndef IGDEMO_ASSETS_CORE_SHADERS_H
#define IGDEMO_ASSETS_CORE_SHADERS_H

#include <igasync/promise.h>
#include <igasync/thread_pool.h>
#include <igdemo/igdemo-app.h>
#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>
#include <memory>

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>> load_core_shaders(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue, wgpu::TextureFormat output_format,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks);

}  // namespace igdemo

#endif
