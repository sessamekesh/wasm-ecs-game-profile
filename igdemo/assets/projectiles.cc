#include <igasync/promise_combiner.h>
#include <igdemo/assets/projectiles.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/projectile.h>
#include <igdemo/render/ctx-components.h>
#include <igdemo/render/geo/sphere.h>
#include <igdemo/render/static-pbr.h>
#include <igdemo/render/world-transform-component.h>

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>>
load_projectile_resources(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks,
    std::shared_ptr<igasync::Promise<bool>> shaderLoadedPromise) {
  SphereGenerator projectile_sphere_gen{20, 12};

  auto sphere_vertices_promise = igasync::Promise<void>::Immediate()->then(
      [projectile_sphere_gen]() {
        return projectile_sphere_gen.get_vertices();
      },
      compute_tasks);
  auto sphere_indices_promise = igasync::Promise<void>::Immediate()->then(
      [projectile_sphere_gen]() { return projectile_sphere_gen.get_indices(); },
      compute_tasks);

  auto final_combiner = igasync::PromiseCombiner::Create();
  auto pbr_shader_loaded_key =
      final_combiner->add(shaderLoadedPromise, main_thread_tasks);
  auto sphere_verts_rsl_key =
      final_combiner->add_consuming(sphere_vertices_promise, compute_tasks);
  auto sphere_indices_rsl_key =
      final_combiner->add_consuming(sphere_indices_promise, compute_tasks);

  return final_combiner->combine(
      [pbr_shader_loaded_key, sphere_verts_rsl_key, sphere_indices_rsl_key, r,
       device, queue](igasync::PromiseCombiner::Result rsl) {
        std::vector<std::string> errors;

        auto pbr_shader_loaded = rsl.get(pbr_shader_loaded_key);

        if (!pbr_shader_loaded) {
          errors.push_back(
              "Static PBR shader not loaded - cannot create projectiles");
        }

        if (errors.size() > 0) {
          return errors;
        }

        auto sphere_verts = rsl.move(sphere_verts_rsl_key);
        auto sphere_indices = rsl.move(sphere_indices_rsl_key);

        auto wv = igecs::WorldView::Thin(r);
        const auto& shader = wv.ctx<CtxStaticPbrPipeline>();

        StaticPbrGeometry sphere(device, queue, sphere_verts, sphere_indices);

        pbr::GPUPbrColorParams basicEnemyProjectileMatDef{};
        basicEnemyProjectileMatDef.albedo = glm::vec3(0.96f, 0.54f, 0.74f);
        basicEnemyProjectileMatDef.metallic = 0.f;
        basicEnemyProjectileMatDef.roughness = 1.f;

        pbr::GPUPbrColorParams basicHeroProjectileMatDef{};
        basicHeroProjectileMatDef.albedo = glm::vec3(0.9f, 0.85f, 0.96f);
        basicHeroProjectileMatDef.metallic = 1.f;
        basicHeroProjectileMatDef.roughness = 1.f;

        StaticPbrMaterial basicEnemyProjectileMaterial(
            device, queue, shader.obj_bgl, basicEnemyProjectileMatDef);
        StaticPbrMaterial basicHeroProjectileMaterial(
            device, queue, shader.obj_bgl, basicHeroProjectileMatDef);

        wv.attach_ctx<CtxProjectileRenderResources>(
            CtxProjectileRenderResources{
                sphere,
                basicEnemyProjectileMaterial,
                basicHeroProjectileMaterial,
            });

        return errors;
      },
      main_thread_tasks);
}

igecs::WorldView::Decl ProjectileRenderUtil::decl() {
  return igecs::WorldView::Decl()
      .ctx_reads<CtxProjectileRenderResources>()
      .ctx_reads<CtxWgpuDevice>()
      .ctx_reads<CtxStaticPbrPipeline>()
      .reads<Projectile>()
      .writes<WorldTransformComponent>()
      .writes<StaticPbrInstance>()
      .writes<OrientationComponent>()
      .writes<StaticPbrModelBindGroup>();
}

bool ProjectileRenderUtil::has_render_resources(igecs::WorldView* wv,
                                                entt::entity e) {
  return wv->has<StaticPbrInstance>(e) && wv->has<WorldTransformComponent>(e) &&
         wv->has<OrientationComponent>(e) && wv->has<ScaleComponent>(e) &&
         wv->has<StaticPbrModelBindGroup>(e);
}

void ProjectileRenderUtil::attach_render_resources(igecs::WorldView* wv,
                                                   entt::entity e) {
  const auto& ctxResources = wv->ctx<CtxProjectileRenderResources>();
  const auto& ctxDevice = wv->ctx<CtxWgpuDevice>();
  const auto& shader = wv->ctx<CtxStaticPbrPipeline>();

  const auto& projectile = wv->read<Projectile>(e);

  wv->attach<StaticPbrInstance>(
      e, StaticPbrInstance{((projectile.type == ProjectileSource::Hero)
                                ? &ctxResources.basicHeroProjectileMaterial
                                : &ctxResources.basicEnemyProjectileMaterial),
                           &ctxResources.projectileSphereGeometry});
  wv->attach<OrientationComponent>(e, OrientationComponent{0.f});
  wv->attach<ScaleComponent>(e, ScaleComponent{0.35f});
  wv->attach<WorldTransformComponent>(e);
  wv->attach<StaticPbrModelBindGroup>(e, ctxDevice.device, ctxDevice.queue,
                                      shader.model_bgl);
}

}  // namespace igdemo
