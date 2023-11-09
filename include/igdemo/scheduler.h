#ifndef IGDEMO_SCHEDULER_H
#define IGDEMO_SCHEDULER_H

#include <igecs/scheduler.h>

namespace igdemo {

igecs::Scheduler build_update_and_render_scheduler(
    const std::vector<std::thread::id>& worker_thread_ids);

}  // namespace igdemo

#endif
