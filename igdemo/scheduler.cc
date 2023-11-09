#include <igdemo/render/tonemap-pass.h>
#include <igdemo/scheduler.h>

namespace igdemo {

igecs::Scheduler build_update_and_render_scheduler(
    const std::vector<std::thread::id>& worker_thread_ids) {
  auto builder = igecs::Scheduler::Builder("IgDemo Frame");
  builder.main_thread_id(std::this_thread::get_id());
  builder.max_spin_time(std::chrono::milliseconds(5000));

  for (int i = 0; i < worker_thread_ids.size(); i++) {
    builder.worker_thread_id(worker_thread_ids[i]);
  }

  auto tonemapping_pass = builder.add_node()
                              .main_thread_only()
                              .with_decl(TonemapPass::decl())
                              .build<TonemapPass>();

  return builder.build();
}

}  // namespace igdemo
