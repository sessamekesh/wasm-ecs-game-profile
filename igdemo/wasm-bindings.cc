#include <emscripten/bind.h>
#include <emscripten/fetch.h>
#include <igdemo/igdemo-app.h>
#include <igdemo/platform/web/raii-fetch.h>

#include <iostream>
#include <sstream>

using namespace emscripten;

static std::unique_ptr<iggpu::AppBase> gAppBase = nullptr;
static std::shared_ptr<igasync::ThreadPool> gThreadPool = nullptr;

igdemo::IgdemoProcTable create_proc_table(
    val dump_profile_cb, val loading_progress_cb,
    std::shared_ptr<igasync::TaskList> process_events_task_list) {
  igdemo::IgdemoProcTable proc_table{};

  proc_table.dumpProfileCb = [dump_profile_cb](std::string json) {
    dump_profile_cb(json);
  };
  proc_table.indicateProgress =
      [loading_progress_cb](igdemo::LoadingProgressMark mark) {
        loading_progress_cb(mark);
      };
  proc_table.loadFileCb = &igdemo::web::read_file;

  return proc_table;
}

void cleanup_app_base() {
  if (gAppBase) {
    gAppBase = nullptr;
  }

  if (gThreadPool) {
    gThreadPool->clear_all_task_lists();
    gThreadPool = nullptr;
  }
}

void create_new_app(std::string canvas_name,
                    std::shared_ptr<igasync::TaskList> main_thread_task_list,
                    igdemo::IgdemoConfig config,
                    igdemo::IgdemoProcTable proc_table, val resolve,
                    val reject) {
  iggpu::AppBase::Create(canvas_name)
      ->consume(
          [resolve, reject, config = std::move(config),
           proc_table = std::move(proc_table),
           main_thread_task_list](auto base_app_create_rsl) {
            if (std::holds_alternative<iggpu::AppBaseCreateError>(
                    base_app_create_rsl)) {
              std::stringstream ss;

              ss << "Failed to create app: "
                 << iggpu::app_base_create_error_text(
                        std::get<iggpu::AppBaseCreateError>(
                            base_app_create_rsl));

              reject(ss.str());
              return;
            }

            gAppBase = std::move(
                std::get<std::unique_ptr<iggpu::AppBase>>(base_app_create_rsl));

            igasync::ThreadPool::Desc thread_pool_desc{};
            if (config.multithreaded) {
              if (config.threadCountOverride > 0) {
                thread_pool_desc.AdditionalThreads =
                    config.threadCountOverride -
                    std::thread::hardware_concurrency() - 1;
              } else {
                thread_pool_desc.AdditionalThreads =
                    -1;  // To account for main thread
              }
              thread_pool_desc.UseHardwareConcurrency = true;
            } else {
              thread_pool_desc.UseHardwareConcurrency = false;
              thread_pool_desc.AdditionalThreads = 0;
            }
            gThreadPool = igasync::ThreadPool::Create(thread_pool_desc);
            auto async_tasks = igasync::TaskList::Create();

            if (config.multithreaded) {
              gThreadPool->add_task_list(async_tasks);
            } else {
              async_tasks = main_thread_task_list;
            }

            auto load_start_time = std::chrono::high_resolution_clock::now();
            auto app_create_rsl = igdemo::IgdemoApp::Create(
                gAppBase.get(), std::move(config), std::move(proc_table),
                gThreadPool->thread_ids(), main_thread_task_list, async_tasks);

            app_create_rsl->consume(
                [main_thread_task_list, load_start_time,
                 config = std::move(config), resolve, reject](auto rsl) {
                  if (std::holds_alternative<igdemo::IgdemoLoadError>(rsl)) {
                    const auto& err = std::get<igdemo::IgdemoLoadError>(rsl);

                    std::stringstream ss;
                    ss << "Failed to load IgDemo application!" << std::endl;

                    ss << "\nAsset load failures:" << std::endl;
                    for (int i = 0; i < err.assetLoadFailures.size(); i++) {
                      ss << " -- " << err.assetLoadFailures[i] << std::endl;
                    }

                    reject(ss.str());
                    return;
                  }

                  resolve(std::move(std::move(
                      std::get<std::unique_ptr<igdemo::IgdemoApp>>(rsl))));
                },
                main_thread_task_list);
          },
          main_thread_task_list);
}

//
// Bindings
//
EMSCRIPTEN_BINDINGS(IgDemoModule) {
  class_<iggpu::AppBase>("AppBase");

  value_object<igdemo::IgdemoConfig>("IgdemoConfig")
      .field("rngSeed", &igdemo::IgdemoConfig::rngSeed)
      .field("numEnemyMobs", &igdemo::IgdemoConfig::numEnemyMobs)
      .field("numHeroes", &igdemo::IgdemoConfig::numHeroes)
      .field("numWarmupFrames", &igdemo::IgdemoConfig::numWarmupFrames)
      .field("numProfiles", &igdemo::IgdemoConfig::numProfiles)
      .field("profileFrameGapSize", &igdemo::IgdemoConfig::profileFrameGapSize)
      .field("renderOutput", &igdemo::IgdemoConfig::renderOutput)
      .field("multithreaded", &igdemo::IgdemoConfig::multithreaded)
      .field("threadCountOverride", &igdemo::IgdemoConfig::threadCountOverride)
      .field("assetRootPath", &igdemo::IgdemoConfig::assetRootPath);

  class_<igdemo::IgdemoApp>("IgdemoApp")
      .function("update_and_render", &igdemo::IgdemoApp::update_and_render);
  class_<igdemo::IgdemoProcTable>("IgdemoProcTable");

  value_object<igasync::TaskList::Desc>("TaskListDesc")
      .field("queueSizeHint", &igasync::TaskList::Desc::QueueSizeHint)
      .field("enqueueListenerSizeHint",
             &igasync::TaskList::Desc::EnqueueListenerSizeHint);
  class_<igasync::TaskList>("TaskList")
      .class_function("Create", &igasync::TaskList::Create)
      .smart_ptr<std::shared_ptr<igasync::TaskList>>("TaskList")
      .function("execute_next", &igasync::TaskList::execute_next);

  enum_<igdemo::LoadingProgressMark>("LoadingProgressMark")
      .value("CharacterModelLoaded",
             igdemo::LoadingProgressMark::CharacterModelLoaded)
      .value("SkyboxGenerated", igdemo::LoadingProgressMark::SkyboxGenerated)
      .value("CoreShadersLoaded",
             igdemo::LoadingProgressMark::CoreShadersLoaded)
      .value("AppLoadFailed", igdemo::LoadingProgressMark::AppLoadFailed)
      .value("AppLoadSuccess", igdemo::LoadingProgressMark::AppLoadSuccess);

  function("create_proc_table", &create_proc_table);
  function("create_new_app", &create_new_app);
  function("cleanup_app_base", &cleanup_app_base);
}
