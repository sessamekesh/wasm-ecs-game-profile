#ifndef IGDEMO_SYSTEMS_FLY_CAMERA_H
#define IGDEMO_SYSTEMS_FLY_CAMERA_H

#include <igecs/world_view.h>

namespace igdemo {

struct FlyCameraSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
