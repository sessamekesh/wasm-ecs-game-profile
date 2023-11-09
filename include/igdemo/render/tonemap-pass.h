#ifndef IGDEMO_RENDER_TONEMAP_PASS_H
#define IGDEMO_RENDER_TONEMAP_PASS_H

#include <igecs/world_view.h>

namespace igdemo {

class TonemapPass {
 public:
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
