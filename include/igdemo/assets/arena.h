#ifndef IGDEMO_ASSETS_ARENA_H
#define IGDEMO_ASSETS_ARENA_H

#include <igasync/promise.h>
#include <igdemo/igdemo-app.h>

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>>
load_arena_resources(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::Promise<bool>> static_shader_loaded_promise,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks);

}

#endif
