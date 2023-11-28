#include <igdemo/logic/spatial-index.h>

#include <glm/gtx/norm.hpp>

namespace igdemo {

GridIndex::GridIndex(float xMin, float xRange, float zMin, float zRange,
                     std::uint32_t subdivisions)
    : xMin_(xMin),
      xRange_(xRange),
      zMin_(zMin),
      zRange_(zRange),
      subdivisions_(subdivisions) {
  cells_.resize(subdivisions * subdivisions);
}

igecs::WorldView::Decl GridIndex::mut_decl() {
  return igecs::WorldView::Decl().writes<GridIndexDataComponent>();
}

igecs::WorldView::Decl GridIndex::decl() {
  return igecs::WorldView::Decl().reads<GridIndexDataComponent>();
}

void GridIndex::insert_or_update(igecs::WorldView* wv, entt::entity entity,
                                 glm::vec2 pos, float radius) {
  if (wv->has<GridIndexDataComponent>(entity)) {
    auto& existing = wv->write<GridIndexDataComponent>(entity);
    int existingX, existingZ;
    int nextX, nextZ;
    get_grid_cells(existing.pos, existingX, existingZ);
    get_grid_cells(pos, nextX, nextZ);

    existing.pos = pos;
    existing.radius = radius;

    if (existingX != nextX || existingZ != nextZ) {
      auto& last_cell = get_cell(existingX, existingZ);
      auto& next_cell = get_cell(nextX, nextZ);

      last_cell.remove(entity);
      next_cell.add(entity);
    }

    return;
  }

  int cellX, cellZ;
  get_grid_cells(pos, cellX, cellZ);
  auto& cell = get_cell(cellX, cellZ);

  wv->attach<GridIndexDataComponent>(entity,
                                     GridIndexDataComponent{pos, radius});
  cell.add(entity);
}

void GridIndex::remove(igecs::WorldView* wv, entt::entity entity) {
  if (!wv->has<GridIndexDataComponent>(entity)) {
    return;
  }

  const auto& entry = wv->read<GridIndexDataComponent>(entity);
  int cellX, cellZ;
  get_grid_cells(entry.pos, cellX, cellZ);
  auto& cell = get_cell(cellX, cellZ);
  cell.remove(entity);
  wv->remove<GridIndexDataComponent>(entity);
}

std::optional<entt::entity> GridIndex::nearest_neighbor(igecs::WorldView* wv,
                                                        glm::vec2 pos) const {
  float nearestDist = zRange_ * xRange_;
  std::optional<entt::entity> nearest = {};
  int startX, startZ;
  get_grid_cells(pos, startX, startZ);

  for (int distance = 1; distance < subdivisions_; distance++) {
    bool found = false;
    for (int x = startX - distance; x <= startX + distance; x++) {
      if (x < 0 || x > subdivisions_) continue;

      for (int z = startZ - distance; z <= startZ + distance; z++) {
        if (z < 0 || z > subdivisions_) continue;

        const auto& cell = get_cell(x, z);
        for (const auto& entry : cell.entries) {
          const auto& data = wv->read<GridIndexDataComponent>(entry);
          auto dist = glm::length2(data.pos - pos);
          if (dist < nearestDist) {
            nearestDist = dist;
            nearest = entry;
            found = true;
          }
        }
      }
    }

    if (found) {
      break;
    }
  }

  return nearest;
}

std::vector<entt::entity> GridIndex::collisions(igecs::WorldView* wv,
                                                glm::vec2 pos,
                                                float radius) const {
  std::vector<entt::entity> collidingEntities;
  int startX, startZ;
  get_grid_cells(pos, startX, startZ);

  int xWidth =
      static_cast<int>(radius / (xRange_ / static_cast<float>(subdivisions_))) +
      1;
  int zWidth =
      static_cast<int>(radius / (zRange_ / static_cast<float>(subdivisions_))) +
      1;

  for (int x = startX - xWidth; x <= startX + xWidth; x++) {
    if (x < 0 || x > subdivisions_) continue;

    for (int z = startZ - zWidth; z <= startZ + zWidth; z++) {
      if (z < 0 || z > subdivisions_) continue;

      const auto& cell = get_cell(x, z);
      for (const auto& entry : cell.entries) {
        const auto& data = wv->read<GridIndexDataComponent>(entry);
        auto dist = glm::length2(data.pos - pos);
        if (dist <= (radius + data.radius) * (radius + data.radius)) {
          collidingEntities.push_back(entry);
        }
      }
    }
  }

  return collidingEntities;
}

const GridIndex::CellContents& GridIndex::get_cell(int xGrid, int zGrid) const {
  auto gridIdx = cell_idx(xGrid, zGrid);

  return cells_[gridIdx];
}

GridIndex::CellContents& GridIndex::get_cell(int xGrid, int zGrid) {
  auto gridIdx = cell_idx(xGrid, zGrid);

  return cells_[gridIdx];
}

size_t GridIndex::cell_idx(int xGrid, int zGrid) const {
  xGrid = glm::clamp(xGrid, 0, static_cast<int>(subdivisions_) - 1);
  zGrid = glm::clamp(zGrid, 0, static_cast<int>(subdivisions_) - 1);
  return xGrid * subdivisions_ + zGrid;
}

void GridIndex::get_grid_cells(glm::vec2 pos, int& xCell, int& zCell) const {
  xCell = (pos.x - xMin_) / xRange_ * static_cast<int>(subdivisions_);
  zCell = (pos.y - zMin_) / zRange_ * static_cast<int>(subdivisions_);
}

void GridIndex::CellContents::add(entt::entity e) { entries.push_back(e); }

void GridIndex::CellContents::remove(entt::entity e) { std::erase(entries, e); }

}  // namespace igdemo
