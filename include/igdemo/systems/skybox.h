#ifndef IGDEMO_SYSTEMS_SKYBOX_H_
#define IGDEMO_SYSTEMS_SKYBOX_H_

#include <igecs/world_view.h>

namespace igdemo {

struct SkyboxRenderSystem {
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
