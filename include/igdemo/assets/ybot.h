#ifndef IGDEMO_ASSETS_YBOT_H
#define IGDEMO_ASSETS_YBOT_H

#include <igasset/ozz_jobs.h>
#include <igasset/ozz_wrappers.h>
#include <igasync/promise.h>
#include <igdemo/igdemo-app.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <vector>

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>> load_ybot_resources(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks);

void attach_ybot_render_resources(igecs::WorldView* wv, entt::entity e);

}  // namespace igdemo

#endif
