#ifndef IGDMEO_LOGIC_SPATIAL_INDEX_H
#define IGDMEO_LOGIC_SPATIAL_INDEX_H

#include <igecs/world_view.h>

#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <optional>

namespace igdemo {

class GridIndex {
 public:
  GridIndex(float xMin, float xRange, float zMin, float zRange,
            std::uint32_t subdivisions);

  static igecs::WorldView::Decl mut_decl();
  static igecs::WorldView::Decl decl();

  void insert_or_update(igecs::WorldView* wv, entt::entity entity,
                        glm::vec2 pos, float radius);
  void remove(igecs::WorldView* wv, entt::entity entity);

  std::optional<entt::entity> nearest_neighbor(igecs::WorldView* wv,
                                               glm::vec2 pos) const;
  std::vector<entt::entity> collisions(igecs::WorldView* wv, glm::vec2 pos,
                                       float radius) const;

  GridIndex() = delete;
  ~GridIndex() = default;
  GridIndex(const GridIndex&) = delete;
  GridIndex& operator=(const GridIndex&) = delete;
  GridIndex(GridIndex&&) = default;
  GridIndex& operator=(GridIndex&&) = default;

 private:
  struct GridIndexDataComponent {
    glm::vec2 pos;
    float radius;
  };

  float xMin_;
  float xRange_;
  float zMin_;
  float zRange_;
  std::uint32_t subdivisions_;

  struct CellContents {
    std::vector<entt::entity> entries;

    void add(entt::entity);
    void remove(entt::entity);
  };
  std::vector<CellContents> cells_;

  void get_grid_cells(glm::vec2 pos, int& xCell, int& zCell) const;
  const CellContents& get_cell(int xGrid, int zGrid) const;
  CellContents& get_cell(int xGrid, int zGrid);

  size_t cell_idx(int xGrid, int zGrid) const;
};

struct CtxSpatialIndex {
  CtxSpatialIndex(float xMin_, float xRange_, float zMin_, float zRange_,
                  std::uint32_t subdivisions)
      : xMin(xMin_),
        xRange(xRange_),
        zMin(zMin_),
        zRange(zRange_),
        heroIndex(xMin_, xRange_, zMin_, zRange_, subdivisions),
        enemyIndex(xMin_, xRange_, zMin_, zRange_, subdivisions) {}

  float xMin;
  float xRange;
  float zMin;
  float zRange;

  GridIndex heroIndex;
  GridIndex enemyIndex;
};

}  // namespace igdemo

#endif
