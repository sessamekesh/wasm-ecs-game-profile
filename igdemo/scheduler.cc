#include <igdemo/logic/locomotion.h>
#include <igdemo/scheduler.h>
#include <igdemo/systems/animation.h>
#include <igdemo/systems/attach-renderables.h>
#include <igdemo/systems/destroy-actor.h>
#include <igdemo/systems/enemy-locomotion.h>
#include <igdemo/systems/fly-camera.h>
#include <igdemo/systems/hero-locomotion.h>
#include <igdemo/systems/locomotion.h>
#include <igdemo/systems/move-projectile.h>
#include <igdemo/systems/pbr-geo-pass.h>
#include <igdemo/systems/skybox.h>
#include <igdemo/systems/spawn-projectiles.h>
#include <igdemo/systems/tonemap-pass.h>
#include <igdemo/systems/update-spatial-index.h>

namespace igdemo {

igecs::Scheduler build_update_and_render_scheduler(
    const std::vector<std::thread::id>& worker_thread_ids) {
  auto builder = igecs::Scheduler::Builder("IgDemo Frame");
  builder.main_thread_id(std::this_thread::get_id());
  builder.max_spin_time(std::chrono::milliseconds(5000));

  for (int i = 0; i < worker_thread_ids.size(); i++) {
    builder.worker_thread_id(worker_thread_ids[i]);
  }

  // TODO (sessamekesh): System to consume damage events, apply death

  auto hero_locomotion = builder.add_node().build<HeroLocomotionSystem>();

  auto enemy_locomotion = builder.add_node()
                              .depends_on(hero_locomotion)
                              .build<UpdateEnemiesSystem>();

  auto spawn_projectiles = builder.add_node()
                               .depends_on(enemy_locomotion)
                               .build<SpawnProjectilesSystem>();

  auto update_projectiles = builder.add_node()
                                .depends_on(spawn_projectiles)
                                .build<MoveProjectileSystem>();

  auto locomotion = builder.add_node()
                        .depends_on(update_projectiles)
                        .build<LocomotionSystem>();

  auto update_spatial_index = builder.add_node()
                                  .depends_on(enemy_locomotion)
                                  .main_thread_only()
                                  .build<UpdateSpatialIndexSystem>();

  // Run at the end of the logic pass (but before rendering pass begins)
  auto destroy_actors = builder.add_node()
                            .main_thread_only()
                            .depends_on(update_spatial_index)
                            .build<DestroyActorSystem>();

  // This system serves as the transition between LOGICAL updates and
  //  RENDER updates - it attaches appropriate render resources to
  //  accompany the matching logic resources.
  auto attach_renderables = builder.add_node()
                                .main_thread_only()
                                .depends_on(locomotion)
                                .depends_on(destroy_actors)
                                .build<AttachRenderablesSystem>();

  auto advance_animation_time = builder.add_node()
                                    .depends_on(attach_renderables)
                                    .build<AdvanceAnimationTimeSystem>();

  auto sample_ozz_animation = builder.add_node()
                                  .depends_on(advance_animation_time)
                                  .build<SampleOzzAnimationSystem>();

  auto transform_ozz_animation_to_model_space =
      builder.add_node()
          .depends_on(sample_ozz_animation)
          .build<TransformOzzAnimationToModelSpaceSystem>();

  auto fly_camera =
      builder.add_node().main_thread_only().build<FlyCameraSystem>();

  auto pbr_upload_scene_buffers =
      builder.add_node()
          .main_thread_only()
          .depends_on(fly_camera)
          // TODO (sessamekesh): .depends_on update camera system
          .build<PbrUploadSceneBuffersSystem>();

  auto pbr_upload_instance_buffers =
      builder.add_node()
          .main_thread_only()
          .depends_on(locomotion)
          .depends_on(transform_ozz_animation_to_model_space)
          .build<PbrUploadPerInstanceBuffersSystem>();

  auto skybox_pass = builder.add_node()
                         .main_thread_only()
                         .depends_on(pbr_upload_scene_buffers)
                         .build<SkyboxRenderSystem>();

  auto pbr_geo_pass = builder.add_node()
                          .main_thread_only()
                          .depends_on(pbr_upload_scene_buffers)
                          .depends_on(pbr_upload_instance_buffers)
                          .depends_on(skybox_pass)
                          .build<PbrGeoPassSystem>();

  auto tonemapping_pass = builder.add_node()
                              .main_thread_only()
                              .depends_on(pbr_geo_pass)
                              .depends_on(skybox_pass)
                              .build<TonemapPass>();

  return builder.build();
}

}  // namespace igdemo
