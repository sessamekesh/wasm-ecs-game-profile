#include <igdemo/igdemo-app.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <filesystem>
#include <future>
#include <iostream>

using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;

int main(int argc, char** argv) {
  bool singlethreaded;
  std::uint32_t rng_seed;
  std::uint32_t num_enemy_mobs;
  std::uint32_t num_heroes;
  std::uint32_t warmup_frame_count;
  std::uint32_t num_profiles;
  std::uint32_t profile_gap_size;
  bool render_output;
  std::string profile_out_dir;
  std::string profile_prefix;

  try {
    CLI::App cli{
        "igdemo - a demo for profiling Indigo game logic on native and WASM "
        "builds",
        "igdemo"};
    cli.add_option("--multithreaded", singlethreaded,
                   "Run this app in single-threaded mode")
        ->default_val(false);
    cli.add_option("--seed", rng_seed,
                   "Seed value for random number generation")
        ->default_val(std::chrono::high_resolution_clock::now()
                          .time_since_epoch()
                          .count() %
                      0xFFFFFFFF)
        ->check(CLI::PositiveNumber);
    cli.add_option("-e,--enemy_count", num_enemy_mobs,
                   "Number of enemy mobs to spawn around the world")
        ->default_val(100)
        ->check(CLI::PositiveNumber);
    cli.add_option("--hero_count", num_heroes)
        ->default_val(4)
        ->check(CLI::PositiveNumber);
    cli.add_option("--warmup_frames", warmup_frame_count)
        ->default_val(200)
        ->check(CLI::NonNegativeNumber);
    cli.add_option("-n,--num_profiles", num_profiles)
        ->default_val(1)
        ->check(CLI::NonNegativeNumber);
    cli.add_option("--profile_gap_size", profile_gap_size)
        ->default_val(0)
        ->check(CLI::NonNegativeNumber);
    cli.add_option("-r,--render_output", render_output)->default_val(true);
    cli.add_option("-o,--profile_out_dir", profile_out_dir)
        ->default_val(std::filesystem::current_path().string())
        ->check(CLI::ExistingDirectory);
    cli.add_option("--profile_prefix", profile_prefix)->default_val("profile");
    CLI11_PARSE(cli, argc, argv);
  } catch (std::runtime_error e) {
    std::cerr << "Failed to parse CLI output: " << e.what() << std::endl;
    return -1;
  }

  auto app_base_rsl = iggpu::AppBase::Create(1800, 1080, "IgDemo");
  if (std::holds_alternative<iggpu::AppBaseCreateError>(app_base_rsl)) {
    std::cerr << "Failed to initialize IgDemo app base" << std::endl;
    return -1;
  }
  std::unique_ptr<iggpu::AppBase> app_base =
      std::move(std::get<std::unique_ptr<iggpu::AppBase>>(app_base_rsl));

  igdemo::IgdemoConfig config{};
  config.multithreaded = !singlethreaded;
  config.numEnemyMobs = num_enemy_mobs;
  config.numHeroes = num_heroes;
  config.numWarmupFrames = warmup_frame_count;
  config.numProfiles = num_profiles;
  config.profileFrameGapSize = profile_gap_size;
  config.renderOutput = render_output;
  config.rngSeed = rng_seed;

  //
  // Proc table (platform details)
  //
  igdemo::IgdemoProcTable proc_table{};
  proc_table.dumpProfileCb = [profile_prefix,
                              profile_out_dir](std::string json) {
    static int profile_id = 0;
    std::string fname = (profile_out_dir + "/" +
                         (profile_prefix + std::to_string(profile_id))) +
                        ".json";
    std::thread([fname = std::move(fname), json = std::move(json)]() {
      std::ofstream fout(fname);
      if (!fout) {
        std::cerr << "Could not write to " << fname << " - profile not written"
                  << std::endl;
        return;
      }

      fout << json;
    }).detach();
  };
  proc_table.loadFileCb = [](std::string file_path) {
    auto rsl = igasync::Promise<
        std::variant<std::string, igdemo::FileReadError>>::Create();
    std::thread(
        [rsl](std::string path) {
          if (!std::filesystem::exists(path)) {
            rsl->resolve(igdemo::FileReadError::FileNotFound);
            return;
          }

          std::ifstream fin(path, std::ios::binary | std::ios::ate);
          if (!fin) {
            rsl->resolve(igdemo::FileReadError::FileNotFound);
            return;
          }

          auto size = fin.tellg();
          fin.seekg(0, std::ios::beg);
          std::string data(size, '\0');
          if (!fin.read(&data[0], size)) {
            rsl->resolve(igdemo::FileReadError::FileNotRead);
            return;
          }

          rsl->resolve(std::move(data));
        },
        file_path)
        .detach();
    return rsl;
  };

  igasync::ThreadPool::Desc thread_pool_desc{};
  if (config.multithreaded) {
    thread_pool_desc.UseHardwareConcurrency = true;
    thread_pool_desc.AdditionalThreads = -1;  // To account for main thread
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
      app_base.get(), std::move(config), std::move(proc_table),
      thread_pool->thread_ids(), main_thread_tasks, async_tasks);

  std::unique_ptr<igdemo::IgdemoApp> demo_app = nullptr;
  app_create_rsl->consume(
      [&demo_app, load_start_time](auto rsl) {
        if (std::holds_alternative<igdemo::IgdemoLoadError>(rsl)) {
          const auto& err = std::get<igdemo::IgdemoLoadError>(rsl);

          std::cerr << "Failed to load IgDemo application!" << std::endl;

          std::cerr << "\nAsset load failures:" << std::endl;
          for (int i = 0; i < err.assetLoadFailures.size(); i++) {
            std::cerr << " -- " << err.assetLoadFailures[i] << std::endl;
          }

          exit(-1);
        }

        demo_app = std::move(std::get<std::unique_ptr<igdemo::IgdemoApp>>(rsl));
        auto load_finish = std::chrono::high_resolution_clock::now();

        auto load_time_seconds =
            FpSeconds(load_finish - load_start_time).count();

        std::cout << "Loaded demo app successfully in " << load_time_seconds
                  << " seconds" << std::endl;
      },
      main_thread_tasks);

  auto last_frame_time = std::chrono::high_resolution_clock::now();
  while (!glfwWindowShouldClose(app_base->Window)) {
    auto now = std::chrono::high_resolution_clock::now();
    auto dt = FpSeconds(now - last_frame_time).count();
    last_frame_time = now;

    if (demo_app) {
      demo_app->update_and_render(dt);
    }

    app_base->process_events();

    if (demo_app) {
      app_base->SwapChain.Present();
    } else {
      while (main_thread_tasks->execute_next()) {
      }
    }
    glfwPollEvents();
  }

  return 0;
}
