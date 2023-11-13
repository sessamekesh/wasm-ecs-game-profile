#ifndef IGDEMO_SYSTEMS_ATTACH_RENDERABLES_H
#define IGDEMO_SYSTEMS_ATTACH_RENDERABLES_H

#include <igecs/world_view.h>

namespace igdemo {

class AttachRenderablesSystem {
 public:
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
