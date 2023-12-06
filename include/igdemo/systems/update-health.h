#ifndef IGDEMO_SYSTEMS_UPDATE_HEALTH_H
#define IGDEMO_SYSTEMS_UPDATE_HEALTH_H

#include <igecs/world_view.h>

namespace igdemo {

struct UpdateHealthSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
