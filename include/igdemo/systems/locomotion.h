#ifndef IGDEMO_SYSTEMS_LOCOMOTION_H
#define IGDEMO_SYSTEMS_LOCOMOTION_H

#include <igecs/world_view.h>

namespace igdemo {

struct LocomotionSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
