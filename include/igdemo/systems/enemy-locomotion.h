#ifndef IGDEMO_SYSTEMS_ENEMY_LOCOMOTION_H
#define IGDEMO_SYSTEMS_ENEMY_LOCOMOTION_H

#include <igdemo/logic/enemy-strategy.h>
#include <igecs/world_view.h>

namespace igdemo {

struct UpdateEnemiesSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);  // TODO (sessamekesh): make async
};

}  // namespace igdemo

#endif
