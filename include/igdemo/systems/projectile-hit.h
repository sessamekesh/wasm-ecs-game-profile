#ifndef IGDEMO_SYSTEMS_PROJECTILE_HIT_H
#define IGDEMO_SYSTEMS_PROJECTILE_HIT_H

#include <igecs/world_view.h>

namespace igdemo {

struct ProjectileHitSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
