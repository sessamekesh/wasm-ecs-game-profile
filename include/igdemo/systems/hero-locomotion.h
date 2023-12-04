#ifndef IGDEMO_SYSTEMS_HERO_LOCOMOTION_H
#define IGDEMO_SYSTEMS_HERO_LOCOMOTION_H

#include <igecs/world_view.h>

namespace igdemo {

struct HeroLocomotionSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
