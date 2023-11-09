#include <igdemo/igdemo-app.h>
#include <igdemo/render/ctx-components.h>
#include <igdemo/scheduler.h>

namespace igdemo {

std::shared_ptr<
    igasync::Promise<std::variant<std::unique_ptr<IgdemoApp>, IgdemoLoadError>>>
IgdemoApp::Create(iggpu::AppBase* app_base, IgdemoConfig config,
                  IgdemoProcTable proc_table,
                  std::vector<std::thread::id> worker_thread_ids,
                  std::shared_ptr<igasync::TaskList> main_thread_tasks,
                  std::shared_ptr<igasync::TaskList> async_tasks) {
  auto registry = entt::registry();
  auto wv = igecs::WorldView::Thin(&registry);
  wv.attach_ctx<CtxWgpuDevice>(app_base->Device, app_base->Queue,
                               app_base->preferred_swap_chain_texture_format(),
                               nullptr);

  auto frame_execution_graph =
      build_update_and_render_scheduler(worker_thread_ids);

  auto app = std::unique_ptr<IgdemoApp>(
      new IgdemoApp(std::move(config), std::move(proc_table),
                    std::move(registry), std::move(frame_execution_graph),
                    app_base, main_thread_tasks, async_tasks));

  return igasync::Promise<std::variant<
      std::unique_ptr<IgdemoApp>, IgdemoLoadError>>::Immediate(std::move(app));
}

void IgdemoApp::update_and_render(float dt) {
  const auto& device = app_base_->Device;
  const auto& queue = app_base_->Queue;
  const auto& swap_chain = app_base_->SwapChain;

  // Prepare frame globals...
  auto wv = igecs::WorldView::Thin(&r_);
  wv.mut_ctx<CtxWgpuDevice>().renderTarget = swap_chain.GetCurrentTextureView();

  // Execute frame graph...
  frame_execution_graph_.execute(async_tasks_, &r_);

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
