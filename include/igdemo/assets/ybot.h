#ifndef IGDEMO_ASSETS_YBOT_H
#define IGDEMO_ASSETS_YBOT_H

#include <igasset/ozz_jobs.h>
#include <igasset/ozz_wrappers.h>
#include <igasync/promise.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <vector>

namespace igdemo {

struct CtxYbotResources {
  std::unique_ptr<igasset::OzzAnimationWithNames> DefeatedAnimation;
  std::unique_ptr<igasset::OzzAnimationWithNames> WalkAnimation;
  std::unique_ptr<igasset::OzzAnimationWithNames> RunAnimation;
  std::unique_ptr<igasset::OzzAnimationWithNames> IdleAnimation;
  std::unique_ptr<ozz::animation::Skeleton> Skeleton;

  wgpu::Buffer VertexBuffer;
  wgpu::Buffer IndexBuffer;
  std::uint32_t NumIndices;
};

std::shared_ptr<igasync::Promise<std::vector<std::string>>> load_ybot_resources(
    entt::registry* r, std::string asset_root_path,
    std::shared_ptr<igasync::ExecutionContext> gpu_tasks,
    std::shared_ptr<igasync::ExecutionContext> ecs_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks);

}  // namespace igdemo

#endif
