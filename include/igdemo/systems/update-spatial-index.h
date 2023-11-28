#ifndef IGDEMO_SYSTEMS_UPDATE_SPATIAL_INDEX_H
#define IGDEMO_SYSTEMS_UPDATE_SPATIAL_INDEX_H

#include <igecs/world_view.h>

namespace igdemo {

struct UpdateSpatialIndexSystem {
  static void init(igecs::WorldView* wv, float xMin, float xRange, float zMin,
                   float zRange, std::uint32_t num_subdivisions);
  static const igecs::WorldView::Decl& decl();
  static void run(igecs::WorldView* wv);
};

}  // namespace igdemo

#endif
