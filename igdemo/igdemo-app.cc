#include <igasync/promise_combiner.h>
#include <igdemo/assets/core-shaders.h>
#include <igdemo/assets/skybox.h>
#include <igdemo/assets/ybot.h>
#include <igdemo/igdemo-app.h>
#include <igdemo/logic/enemy.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/logic/hero.h>
#include <igdemo/platform/keyboard-mouse-input-emitter.h>
#include <igdemo/render/camera.h>
#include <igdemo/render/ctx-components.h>
#include <igdemo/scheduler.h>
#include <igdemo/systems/animation.h>
#include <igdemo/systems/pbr-geo-pass.h>

#include <random>

namespace {

void create_main_camera(igecs::WorldView* wv) {
  auto e = wv->create();
  wv->attach<igdemo::CameraComponent>(
      e, igdemo::CameraComponent{/* position */
                                 glm::vec3(0.f, 4.5f, -150.f),
                                 /* theta */
                                 0.f,
                                 /* phi */
                                 0.f,
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
                            /* sunColor */ glm::vec3(100.f, 100.f, 100.f),
                            /* ambientCoefficient */ 0.0001f});
  wv.attach_ctx<CtxHdrPassOutput>(app_base->Device, app_base->Width,
                                  app_base->Height);

  // I/O...
  wv.attach_ctx<CtxInputEmitter>(CtxInputEmitter{
      std::make_unique<KeyboardMouseInputEmitter>(app_base->Window)});

  // Setup logical level state...
  // TODO (sessamekesh): Generate from config
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    gen.seed(config.rngSeed);
    std::uniform_real_distribution<> pos_distribution(-200.f, 200.f);
    std::uniform_int_distribution<> enemy_strategy_distribution(0, 2);
    std::uniform_int_distribution<> hero_strategy_distribution(0, 2);
    std::uniform_real_distribution<> rot_distribution(0.f, 3.14159f * 2.f);

    for (int i = 0; i < config.numEnemyMobs; i++) {
      glm::vec2 spawn_pos(pos_distribution(gen), pos_distribution(gen));
      float orientation = rot_distribution(gen);
      int strategy_int = enemy_strategy_distribution(gen);

      EnemyStrategy strat;
      switch (strategy_int) {
        case 0:
          strat = EnemyStrategy::BlitzNearestHero;
          break;
        case 1:
          strat = EnemyStrategy::WanderLikeAChuckleFuck;
          break;
        case 2:
        default:
          strat = EnemyStrategy::RespondIfProvoked;
          break;
      }

      enemy::create_enemy_entity(&wv, strat, config.rngSeed + i, spawn_pos,
                                 orientation, igdemo::ModelType::YBOT, 0.01f);
    }

    for (int i = 0; i < config.numHeroes; i++) {
      glm::vec2 spawn_pos(pos_distribution(gen), pos_distribution(gen));
      float orientation = rot_distribution(gen);
      int strategy_int = hero_strategy_distribution(gen);

      HeroStrategy strat;
      switch (strategy_int) {
        case 0:
          strat = HeroStrategy::KiteForDays;
          break;
        case 1:
          strat = HeroStrategy::SprayNPray;
          break;
        case 2:
        default:
          strat = HeroStrategy::HoldYourGround;
          break;
      }

      create_hero_entity(&wv, strat, config.rngSeed + i, spawn_pos, orientation,
                         igdemo::ModelType::YBOT, 0.0175f);
    }
  }

  // Load stuff from the network and initialize resources...
  auto combiner = igasync::PromiseCombiner::Create();

  auto load_shaders_promises = load_core_shaders(
      proc_table, registry.get(), config.assetRootPath + "resources/",
      app_base->Device, app_base->Queue,
      app_base->preferred_swap_chain_texture_format(), main_thread_tasks,
      async_tasks);
  auto& shaders_promise = load_shaders_promises.result;
  shaders_promise->on_resolve(
      [proc_table](const auto&) {
        proc_table.indicateProgress(LoadingProgressMark::CoreShadersLoaded);
      },
      main_thread_tasks);

  auto load_character_promise = load_ybot_resources(
      proc_table, registry.get(), config.assetRootPath + "resources/",
      app_base->Device, app_base->Queue, main_thread_tasks, async_tasks,
      shaders_promise->then([](const auto&) {}, main_thread_tasks));
  shaders_promise->on_resolve(
      [proc_table](const auto&) {
        proc_table.indicateProgress(LoadingProgressMark::CharacterModelLoaded);
      },
      main_thread_tasks);

  auto ybot_load_errors_key =
      combiner->add(load_character_promise, main_thread_tasks);
  auto shaders_load_errorskey =
      combiner->add(shaders_promise, main_thread_tasks);

  auto load_skybox_promise = load_skybox(
      proc_table, registry.get(), config.assetRootPath + "resources/",
      app_base->Device, app_base->Queue, load_shaders_promises.pbrShaderLoaded,
      main_thread_tasks, async_tasks);
  shaders_promise->on_resolve(
      [proc_table](const auto&) {
        proc_table.indicateProgress(LoadingProgressMark::SkyboxGenerated);
      },
      main_thread_tasks);

  auto skybox_load_errorskey =
      combiner->add(load_skybox_promise, main_thread_tasks);

  auto frame_execution_graph_key = combiner->add_consuming(
      main_thread_tasks->run(build_update_and_render_scheduler,
                             worker_thread_ids),
      main_thread_tasks);

  // Little hack to get registry from this scope into the return scope
  //  since igasync doesn't currently support mutable lambdas.
  auto reg_promise_key = combiner->add_consuming(
      igasync::Promise<std::unique_ptr<entt::registry>>::Immediate(
          std::move(registry)),
      main_thread_tasks);

  return combiner->combine(
      [config, proc_table, reg_promise_key, ybot_load_errors_key,
       shaders_load_errorskey, skybox_load_errorskey, frame_execution_graph_key,
       app_base, main_thread_tasks,
       async_tasks](igasync::PromiseCombiner::Result rsl)
          -> std::variant<std::unique_ptr<IgdemoApp>, IgdemoLoadError> {
        const auto& ybot_load_errors = rsl.get(ybot_load_errors_key);
        const auto& shader_load_errors = rsl.get(shaders_load_errorskey);
        const auto& skybox_load_errors = rsl.get(skybox_load_errorskey);

        if (ybot_load_errors.size() > 0 || shader_load_errors.size() > 0 ||
            skybox_load_errors.size() > 0) {
          std::cerr << "Error loading scene!" << std::endl;
          for (const auto& ybot_err : ybot_load_errors) {
            std::cerr << "-- YBot: " << ybot_err << std::endl;
          }
          for (const auto& ybot_err : shader_load_errors) {
            std::cerr << "-- Shader: " << ybot_err << std::endl;
          }
          for (const auto& ybot_err : skybox_load_errors) {
            std::cerr << "-- Skybox: " << ybot_err << std::endl;
          }
          proc_table.indicateProgress(LoadingProgressMark::AppLoadFailed);
          return nullptr;
        }

        auto frame_execution_graph = rsl.move(frame_execution_graph_key);
        auto registry = rsl.move(reg_promise_key);

        proc_table.indicateProgress(LoadingProgressMark::AppLoadSuccess);

        auto app = std::unique_ptr<IgdemoApp>(
            new IgdemoApp(config, proc_table, std::move(registry),
                          std::move(frame_execution_graph), app_base,
                          main_thread_tasks, async_tasks));
        return app;
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
