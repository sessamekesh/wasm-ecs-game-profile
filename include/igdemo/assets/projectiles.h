#ifndef IGDEMO_ASSETS_PROJECTILES_H
#define IGDEMO_ASSETS_PROJECTILES_H

#include <igasync/promise.h>
#include <igdemo/igdemo-app.h>
#include <igdemo/render/static-pbr.h>
#include <igecs/world_view.h>

#include <string>
#include <vector>

namespace igdemo {

struct CtxProjectileRenderResources {
  igdemo::StaticPbrGeometry projectileSphereGeometry;

  igdemo::StaticPbrMaterial basicEnemyProjectileMaterial;
  igdemo::StaticPbrMaterial basicHeroProjectileMaterial;
};

std::shared_ptr<igasync::Promise<std::vector<std::string>>>
load_projectile_resources(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks,
    std::shared_ptr<igasync::Promise<bool>> shaderLoadedPromise);

struct ProjectileRenderUtil {
  static igecs::WorldView::Decl decl();
  static bool has_render_resources(igecs::WorldView* wv, entt::entity e);
  static void attach_render_resources(igecs::WorldView* wv, entt::entity e);
};

}  // namespace igdemo

#endif
