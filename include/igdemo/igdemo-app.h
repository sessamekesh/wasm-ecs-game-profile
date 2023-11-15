#ifndef IGDEMO_IGDEMO_APP_H
#define IGDEMO_IGDEMO_APP_H

#include <igasync/promise.h>
#include <igasync/thread_pool.h>
#include <igecs/scheduler.h>
#include <igecs/world_view.h>
#include <iggpu/app_base.h>

#include <entt/entt.hpp>
#include <variant>

namespace igdemo {

struct IgdemoConfig {
  /**
   * @brief RNG seed - using the same value between 2 runs should show the same
   *  result
   */
  std::uint32_t rngSeed;

  /** @brief Number of enemy mobs to spawn in the scene */
  std::uint32_t numEnemyMobs;

  /** @brief Number of heros to spawn in the scene */
  std::uint32_t numHeroes;

  /**
   * @brief Number of frames that should be processed prior to capturing a
   * profile
   */
  std::uint32_t numWarmupFrames;

  /** @brief Number of profiles to capture */
  std::uint32_t numProfiles;

  /** @brief Gap (in frames) between captured profiles */
  std::uint32_t profileFrameGapSize;

  /**
   * @brief True to render the game, false to only handle processing and
   *  generate a profile
   */
  bool renderOutput;

  /**
   * @brief True to allow multithreading (worker threads), false to process
   *  everything on the main thread
   */
  bool multithreaded;

  /**
   * @brief Base path to read resources from
  */
  std::string assetRootPath;
};

enum class FileReadError { FileNotFound, FileNotRead };

enum class LoadingProgressMark {
  CharacterModelLoaded,
  SkyboxGenerated,
  CoreShadersLoaded,

  AppLoadFailed,
  AppLoadSuccess
};

struct IgdemoProcTable {
  /** @brief Callback to dump the collected profile(s) */
  std::function<void(std::string)> dumpProfileCb;

  /** @brief Method to asynchronously load a file, given a path */
  std::function<std::shared_ptr<igasync::Promise<
      std::variant<std::string, FileReadError>>>(std::string file_path)>
      loadFileCb;

  /** @brief Callback to invoke on various loading progress milestones */
  std::function<void(LoadingProgressMark)> indicateProgress;
};

struct IgdemoLoadError {
  std::vector<std::string> assetLoadFailures;
};

class IgdemoApp {
 public:
  static std::shared_ptr<igasync::Promise<
      std::variant<std::unique_ptr<IgdemoApp>, IgdemoLoadError>>>
  Create(iggpu::AppBase* app_base, IgdemoConfig config,
         IgdemoProcTable proc_table,
         std::vector<std::thread::id> worker_thread_ids,
         std::shared_ptr<igasync::TaskList> main_thread_tasks,
         std::shared_ptr<igasync::TaskList> async_tasks);

  void update_and_render(float dt);

  IgdemoApp(const IgdemoApp&) = delete;
  IgdemoApp& operator=(const IgdemoApp&) = delete;
  IgdemoApp(IgdemoApp&&) = default;
  IgdemoApp& operator=(IgdemoApp&&) = default;
  ~IgdemoApp() = default;

 private:
  IgdemoApp(IgdemoConfig config, IgdemoProcTable proc_table,
            std::unique_ptr<entt::registry> r,
            igecs::Scheduler frame_execution_graph, iggpu::AppBase* app_base,
            std::shared_ptr<igasync::TaskList> main_thread_tasks,
            std::shared_ptr<igasync::TaskList> async_tasks)
      : config_(std::move(config)),
        proc_table_(std::move(proc_table)),
        r_(std::move(r)),
        frame_execution_graph_(std::move(frame_execution_graph)),
        app_base_(app_base),
        main_thread_tasks_(main_thread_tasks),
        async_tasks_(async_tasks),
        frame_id_(0u),
        remaining_profiles_(config.numProfiles) {}

  IgdemoConfig config_;
  IgdemoProcTable proc_table_;
  std::unique_ptr<entt::registry> r_;

  igecs::Scheduler frame_execution_graph_;
  iggpu::AppBase* app_base_;

  std::shared_ptr<igasync::TaskList> main_thread_tasks_;
  std::shared_ptr<igasync::TaskList> async_tasks_;

  uint32_t frame_id_;
  int remaining_profiles_;
};

}  // namespace igdemo

#endif
