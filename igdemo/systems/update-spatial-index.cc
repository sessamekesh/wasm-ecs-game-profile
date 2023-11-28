#include <igdemo/logic/enemy.h>
#include <igdemo/logic/hero.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/spatial-index.h>
#include <igdemo/systems/update-spatial-index.h>

namespace {
const float kHeroRadius = 0.35f;
const float kEnemyRadius = 0.25f;
}  // namespace

namespace igdemo {

void UpdateSpatialIndexSystem::init(igecs::WorldView* wv, float xMin,
                                    float xRange, float zMin, float zRange,
                                    std::uint32_t num_subdivisions) {
  wv->attach_ctx<CtxSpatialIndex>(xMin, xRange, zMin, zRange, num_subdivisions);
}

const igecs::WorldView::Decl& UpdateSpatialIndexSystem::decl() {
  static igecs::WorldView::Decl decl =
      igecs::WorldView::Decl()
          .ctx_writes<CtxSpatialIndex>()
          .reads<HeroTag>()
          .reads<enemy::EnemyTag>()
          .reads<PositionComponent>()
          .merge_in_decl(GridIndex::mut_decl());

  return decl;
}

void UpdateSpatialIndexSystem::run(igecs::WorldView* wv) {
  auto& ctxSpatialIndex = wv->mut_ctx<CtxSpatialIndex>();

  {
    auto hero_view = wv->view<const HeroTag, const PositionComponent>();

    for (auto [e, pos] : hero_view.each()) {
      ctxSpatialIndex.heroIndex.insert_or_update(wv, e, pos.map_position,
                                                 kHeroRadius);
    }
  }

  {
    auto enemy_view =
        wv->view<const enemy::EnemyTag, const PositionComponent>();
    for (auto [e, pos] : enemy_view.each()) {
      ctxSpatialIndex.enemyIndex.insert_or_update(wv, e, pos.map_position,
                                                  kEnemyRadius);
    }
  }
}

}  // namespace igdemo
