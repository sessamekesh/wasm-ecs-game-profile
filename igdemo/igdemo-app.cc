#include <igasync/promise_combiner.h>
#include <igdemo/assets/core-shaders.h>
#include <igdemo/assets/ybot.h>
#include <igdemo/igdemo-app.h>
#include <igdemo/logic/enemy.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/render/camera.h>
#include <igdemo/render/ctx-components.h>
#include <igdemo/scheduler.h>
#include <igdemo/systems/animation.h>
#include <igdemo/systems/pbr-geo-pass.h>

namespace {

void create_main_camera(igecs::WorldView* wv) {
  auto e = wv->create();
  wv->attach<igdemo::CameraComponent>(
      e, igdemo::CameraComponent{/* position */
                                 glm::vec3(0.f, 100.f, -100.f),
                                 /* lookAt */
                                 glm::vec3(0.f, 100.f, 0.f),
                                 /* up */
                                 glm::vec3(0.f, 1.f, 0.f),
                                 /* fovy */
                                 glm::radians(85.f),
                                 /* nearPlane + farPlane */
                                 0.01f, 10000.f});
  wv->attach_ctx<igdemo::CtxActiveCamera>(igdemo::CtxActiveCamera{e});
}

}  // namespace

namespace igdemo {

std::shared_ptr<
    igasync::Promise<std::variant<std::unique_ptr<IgdemoApp>, IgdemoLoadError>>>
IgdemoApp::Create(iggpu::AppBase* app_base, IgdemoConfig config,
                  IgdemoProcTable proc_table,
                  std::vector<std::thread::id> worker_thread_ids,
                  std::shared_ptr<igasync::TaskList> main_thread_tasks,
                  std::shared_ptr<igasync::TaskList> async_tasks) {
  auto registry = std::make_unique<entt::registry>();

  // Synchronous context setup for systems globals...
  auto wv = igecs::WorldView::Thin(registry.get());
  wv.attach_ctx<CtxWgpuDevice>(app_base->Device, app_base->Queue,
                               app_base->preferred_swap_chain_texture_format(),
                               nullptr);
  wv.attach_ctx<CtxGeneral3dBuffers>(app_base->Device, app_base->Queue);
  init_animation_systems(&wv);
  wv.attach_ctx<CtxFrameTime>();
  ::create_main_camera(&wv);
  wv.attach_ctx<CtxGeneralSceneParams>(
      CtxGeneralSceneParams{/* sunDirection */ glm::vec3(1.f, -4.f, 1.f),
                            /* sunColor */ glm::vec3(10.f, 10.f, 10.f),
                            /* ambientCoefficient */ 0.0001f});
  wv.attach_ctx<CtxHdrPassOutput>(app_base->Device, app_base->Width,
                                  app_base->Height);

  // Setup logical level state...
  // TODO (sessamekesh): Generate from config
  auto enemy =
      enemy::create_enemy_entity(&wv, glm::vec2(0.f, 0.f), glm::radians(180.f),
                                 igdemo::ModelType::YBOT, 1.f);

  // Load stuff from the network and initialize resources...
  auto combiner = igasync::PromiseCombiner::Create();

  auto shaders_promise = load_core_shaders(
      proc_table, registry.get(), "resources/", app_base->Device,
      app_base->Queue, app_base->preferred_swap_chain_texture_format(),
      main_thread_tasks, async_tasks);

  auto ybot_load_errors_key = combiner->add(
      load_ybot_resources(
          proc_table, registry.get(), "resources/", app_base->Device,
          app_base->Queue, main_thread_tasks, async_tasks,
          shaders_promise->then([](const auto&) {}, async_tasks)),
      async_tasks);

  auto shaders_load_errorskey = combiner->add(shaders_promise, async_tasks);

  auto frame_execution_graph_key = combiner->add_consuming(
      main_thread_tasks->run(build_update_and_render_scheduler,
                             worker_thread_ids),
      async_tasks);

  // Little hack to get registry from this scope into the return scope
  //  since igasync doesn't currently support mutable lambdas.
  auto reg_promise_key = combiner->add_consuming(
      igasync::Promise<std::unique_ptr<entt::registry>>::Immediate(
          std::move(registry)),
      main_thread_tasks);

  return combiner->combine(
      [config, proc_table, reg_promise_key, ybot_load_errors_key,
       shaders_load_errorskey, frame_execution_graph_key, app_base,
       main_thread_tasks, async_tasks](igasync::PromiseCombiner::Result rsl)
          -> std::variant<std::unique_ptr<IgdemoApp>, IgdemoLoadError> {
        auto frame_execution_graph = rsl.move(frame_execution_graph_key);
        auto registry = rsl.move(reg_promise_key);

        return std::unique_ptr<IgdemoApp>(
            new IgdemoApp(config, proc_table, std::move(registry),
                          std::move(frame_execution_graph), app_base,
                          main_thread_tasks, async_tasks));
      },
      main_thread_tasks);
}

void IgdemoApp::update_and_render(float dt) {
  const auto& device = app_base_->Device;
  const auto& queue = app_base_->Queue;
  const auto& swap_chain = app_base_->SwapChain;

  // Prepare frame globals...
  auto wv = igecs::WorldView::Thin(r_.get());
  wv.mut_ctx<CtxWgpuDevice>().renderTarget = swap_chain.GetCurrentTextureView();
  wv.mut_ctx<CtxFrameTime>().secondsSinceLastFrame = dt;

  // Execute frame graph...
  frame_execution_graph_.execute(async_tasks_, r_.get());

  // Profiling...
  frame_id_++;
  if (remaining_profiles_ > 0 && frame_id_ > config_.numWarmupFrames) {
    if ((frame_id_ - config_.numWarmupFrames) %
            (config_.profileFrameGapSize + 1) ==
        0) {
      proc_table_.dumpProfileCb(frame_execution_graph_.dump_profile(false));
      remaining_profiles_--;
    }
  }

  // Flush main thread tasks before continuing...
  // TODO (sessamekesh): Replace this with a time-limited flush?
  while (main_thread_tasks_->execute_next()) {
  }
}

}  // namespace igdemo
