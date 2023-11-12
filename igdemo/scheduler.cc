#include <igdemo/render/pbr-geo-pass.h>
#include <igdemo/render/tonemap-pass.h>
#include <igdemo/scheduler.h>
#include <igdemo/systems/animation.h>

namespace igdemo {

igecs::Scheduler build_update_and_render_scheduler(
    const std::vector<std::thread::id>& worker_thread_ids) {
  auto builder = igecs::Scheduler::Builder("IgDemo Frame");
  builder.main_thread_id(std::this_thread::get_id());
  builder.max_spin_time(std::chrono::milliseconds(5000));

  for (int i = 0; i < worker_thread_ids.size(); i++) {
    builder.worker_thread_id(worker_thread_ids[i]);
  }

  // TODO (sessamekesh): System to update projectiles, spawn collision events
  // TODO (sessamekesh): System to handle projectile collision events
  // TODO (sessamekesh): System to consume damage events, apply death
  // TODO (sessamekesh): System to move around enemy mobs
  // TODO (sessamekesh): System to decide hero action
  // TODO (sessamekesh): System to fire hero projectiles
  // TODO (sessamekesh): System to update world positions of entities
  // TODO (sessamekesh): HDR render pass system
  // TODO (sessamekesh): Finish implementation of tonemapping pass

  auto advance_animation_time =
      builder.add_node().build<AdvanceAnimationTimeSystem>();

  auto sample_ozz_animation = builder.add_node()
                                  .depends_on(advance_animation_time)
                                  .build<SampleOzzAnimationSystem>();

  auto transform_ozz_animation_to_model_space =
      builder.add_node()
          .depends_on(sample_ozz_animation)
          .build<TransformOzzAnimationToModelSpaceSystem>();

  auto pbr_upload_buffers =
      builder.add_node()
          .main_thread_only()
          .depends_on(transform_ozz_animation_to_model_space)
          .build<PbrUploadPerInstanceBuffersSystem>();

  auto pbr_geo_pass = builder.add_node()
                          .main_thread_only()
                          .depends_on(pbr_upload_buffers)
                          .build<PbrGeoPassSystem>();

  auto tonemapping_pass = builder.add_node()
                              .main_thread_only()
                              .depends_on(pbr_geo_pass)
                              .build<TonemapPass>();

  return builder.build();
}

}  // namespace igdemo
