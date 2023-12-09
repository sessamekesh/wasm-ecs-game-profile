#include <igdemo/assets/arena.h>
#include <igdemo/logic/levelmetadata.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/render/geo/quad.h>
#include <igdemo/render/static-pbr.h>
#include <igdemo/render/world-transform-component.h>

namespace {
struct CtxArenaRenderResources {
  igdemo::StaticPbrGeometry floorGeo;
  igdemo::StaticPbrMaterial floorMaterial;
};
}  // namespace

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>>
load_arena_resources(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::Promise<bool>> static_shader_loaded_promise,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks) {
  return static_shader_loaded_promise->then(
      [device, queue,
       r](const bool& is_static_shader_loaded) -> std::vector<std::string> {
        std::vector<std::string> errors;

        if (!is_static_shader_loaded) {
          errors.push_back("Static shader not loaded");
        }

        if (!errors.empty()) {
          return errors;
        }

        auto wv = igecs::WorldView::Thin(r);

        const auto& ctxLevelMeta = wv.ctx<CtxLevelMetadata>();
        const auto& ctxPipeline = wv.ctx<CtxStaticPbrPipeline>();

        Quad floor_quad{};
        floor_quad.depth = ctxLevelMeta.mapXRange;
        floor_quad.width = ctxLevelMeta.mapZRange;

        auto floor_verts = floor_quad.get_vertices();
        auto floor_indices = floor_quad.get_indices();

        StaticPbrGeometry floor_geo(device, queue, floor_verts, floor_indices);

        pbr::GPUPbrColorParams floor_material_desc{};
        floor_material_desc.albedo = glm::vec3(0.15f, 0.25f, 0.35f);
        floor_material_desc.metallic = 0.2f;
        floor_material_desc.roughness = 0.75f;

        StaticPbrMaterial floor_material(device, queue, ctxPipeline.obj_bgl,
                                         floor_material_desc);

        const auto& ctxArenaRenderResources =
            wv.attach_ctx<CtxArenaRenderResources>(floor_geo, floor_material);

        auto e = wv.create();
        wv.attach<StaticPbrInstance>(e, &ctxArenaRenderResources.floorMaterial,
                                     &ctxArenaRenderResources.floorGeo);
        wv.attach<OrientationComponent>(e, 0.f);
        wv.attach<ScaleComponent>(e, 1.f);
        wv.attach<WorldTransformComponent>(e);
        wv.attach<StaticPbrModelBindGroup>(e, device, queue,
                                           ctxPipeline.model_bgl);
        wv.attach<PositionComponent>(
            e, glm::vec2(ctxLevelMeta.mapXMin + ctxLevelMeta.mapXRange / 2.f,
                         ctxLevelMeta.mapZMin + ctxLevelMeta.mapZRange / 2.f));

        return errors;
      },
      main_thread_tasks);
}

}  // namespace igdemo
