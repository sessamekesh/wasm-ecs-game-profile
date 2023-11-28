#ifndef IGDEMO_SYSTEMS_DESTROY_ACTOR_H
#define IGDEMO_SYSTEMS_DESTROY_ACTOR_H

#include <igecs/world_view.h>

namespace igdemo {

struct EvtDestroyActor {
  entt::entity e;
};

struct DestroyActorSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
