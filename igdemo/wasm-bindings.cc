#include <emscripten/bind.h>
#include <emscripten/fetch.h>
#include <igdemo/igdemo-app.h>
#include <igdemo/platform/web/raii-fetch.h>

#include <iostream>
#include <sstream>

using namespace emscripten;

static std::unique_ptr<iggpu::AppBase> gAppBase = nullptr;

// Hack... Replace this with a settimeout(0) or promise.resolve().then or
//  something.
class ImmediateExecutionContext : public igasync::ExecutionContext {
 public:
  void schedule(std::unique_ptr<igasync::Task> task) override { task->run(); }
};

igdemo::IgdemoProcTable create_proc_table(val dump_profile_cb) {
  igdemo::IgdemoProcTable proc_table{};

  proc_table.dumpProfileCb = [dump_profile_cb](std::string json) {
    dump_profile_cb(json);
  };
  proc_table.loadFileCb = &igdemo::web::read_file;

  return proc_table;
}

void create_new_app(std::string canvas_name, igdemo::IgdemoConfig config,
                    igdemo::IgdemoProcTable proc_table, val resolve,
                    val reject) {
  auto immediate_context = std::make_shared<ImmediateExecutionContext>();
  iggpu::AppBase::Create(canvas_name)
      ->consume(
          [resolve, reject, config = std::move(config),
           proc_table = std::move(proc_table),
           immediate_context](auto base_app_create_rsl) {
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
              thread_pool_desc.UseHardwareConcurrency = true;
              thread_pool_desc.AdditionalThreads =
                  -1;  // To account for main thread
            } else {
              thread_pool_desc.UseHardwareConcurrency = false;
              thread_pool_desc.AdditionalThreads = 0;
            }
            auto thread_pool = igasync::ThreadPool::Create(thread_pool_desc);
            auto main_thread_tasks = igasync::TaskList::Create();
            auto async_tasks = igasync::TaskList::Create();

            if (config.multithreaded) {
              thread_pool->add_task_list(async_tasks);
            } else {
              async_tasks = main_thread_tasks;
            }

            auto load_start_time = std::chrono::high_resolution_clock::now();
            auto app_create_rsl = igdemo::IgdemoApp::Create(
                gAppBase.get(), std::move(config), std::move(proc_table),
                thread_pool->thread_ids(), main_thread_tasks, async_tasks);

            app_create_rsl->consume(
                [immediate_context, load_start_time, config = std::move(config),
                 resolve, reject](auto rsl) {
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
                immediate_context);
          },
          immediate_context);
}

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
      .field("multithreaded", &igdemo::IgdemoConfig::multithreaded);

  class_<igdemo::IgdemoApp>("IgdemoApp")
      .function("update_and_render", &igdemo::IgdemoApp::update_and_render);
  class_<igdemo::IgdemoProcTable>("IgdemoProcTable");

  function("create_proc_table", &create_proc_table);
  function("create_new_app", &create_new_app);
}
