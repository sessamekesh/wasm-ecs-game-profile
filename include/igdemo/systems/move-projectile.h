#ifndef IGDEMO_SYSTEMS_MOVE_PROJECTILE_H
#define IGDEMO_SYSTEMS_MOVE_PROJECTILE_H

#include <igasync/promise.h>
#include <igasync/task_list.h>
#include <igecs/world_view.h>

namespace igdemo {

struct MoveProjectileSystem {
  static const igecs::WorldView::Decl& decl();
  static std::shared_ptr<igasync::Promise<void>> run(
      igecs::WorldView* wv, std::shared_ptr<igasync::TaskList> main_thread,
      std::shared_ptr<igasync::TaskList> any_thread,
      std::function<void(igasync::TaskProfile profile)> profile_cb);
};

}  // namespace igdemo

#endif
