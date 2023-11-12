#ifndef IGDEMO_SYSTEMS_SAMPLE_ANIMATION_H
#define IGDEMO_SYSTEMS_SAMPLE_ANIMATION_H

#include <igasync/promise.h>
#include <igasync/task_list.h>
#include <igecs/world_view.h>

namespace igdemo {

class AdvanceAnimationTimeSystem {
 public:
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

class SampleOzzAnimationSystem {
 public:
  static void set_entities_per_task(igecs::WorldView* wv,
                                    std::uint32_t chunk_size);
  static const igecs::WorldView::Decl& decl();
  static std::shared_ptr<igasync::Promise<void>> run(
      igecs::WorldView* wv, std::shared_ptr<igasync::TaskList> main_thread,
      std::shared_ptr<igasync::TaskList> any_thread,
      std::function<void(igasync::TaskProfile profile)> profile_cb);
};

class TransformOzzAnimationToModelSpaceSystem {
 public:
  static void set_entities_per_task(igecs::WorldView* wv,
                                    std::uint32_t chunk_size);
  static const igecs::WorldView::Decl& decl();
  static std::shared_ptr<igasync::Promise<void>> run(
      igecs::WorldView* wv, std::shared_ptr<igasync::TaskList> main_thread,
      std::shared_ptr<igasync::TaskList> any_thread,
      std::function<void(igasync::TaskProfile profile)> profile_cb);
};

void init_animation_systems(igecs::WorldView* wv);

}  // namespace igdemo

#endif
