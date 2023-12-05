#ifndef IGDEMO_SYSTEMS_SPAWN_PROJECTILES_H
#define IGDEMO_SYSTEMS_SPAWN_PROJECTILES_H

#include <igecs/world_view.h>

namespace igdemo {

struct SpawnProjectilesSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
