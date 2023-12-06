#include <igdemo/logic/framecommon.h>
#include <igdemo/logic/hero.h>
#include <igdemo/logic/levelmetadata.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/projectile.h>
#include <igdemo/logic/spatial-index.h>
#include <igdemo/systems/hero-locomotion.h>

#include <glm/gtc/constants.hpp>
#include <random>

namespace igdemo {

struct SprayNPrayTarget {
  glm::vec2 currentTarget;
  float timeToSwitch;
  std::uint32_t rngOffset;
};

const float kKiteAwaySpeed = 4.5f;

const float kSpraySwitchTime = 3.f;

const igecs::WorldView::Decl& HeroLocomotionSystem::decl() {
  static igecs::WorldView::Decl d = igecs::WorldView::Decl()
                                        .merge_in_decl(GridIndex::decl())
                                        .ctx_reads<CtxSpatialIndex>()
                                        .ctx_reads<CtxFrameTime>()
                                        .ctx_reads<CtxLevelMetadata>()
                                        .reads<HeroTag>()
                                        .reads<HeroStrategyComponent>()
                                        .writes<SprayNPrayTarget>()
                                        .writes<RenderableComponent>()
                                        .writes<PositionComponent>()
                                        .writes<OrientationComponent>()
                                        .writes<ProjectileFiringIntent>();

  return d;
}

static void maybe_set_animation_state(igecs::WorldView* wv, entt::entity e,
                                      AnimationType animation_type) {
  if (wv->has<RenderableComponent>(e)) {
    auto& renderable = wv->write<RenderableComponent>(e);
    renderable.animation = animation_type;
  }
}

static void kite(igecs::WorldView* wv, entt::entity e, float dt,
                 const CtxSpatialIndex& spatial_index, PositionComponent& pos,
                 OrientationComponent& orientation) {
  auto optNearestEnemy =
      spatial_index.enemyIndex.nearest_neighbor(wv, pos.map_position);

  if (!optNearestEnemy || !wv->valid(*optNearestEnemy) ||
      !wv->has<PositionComponent>(*optNearestEnemy)) {
    // Nobody to kite, just hang out
    maybe_set_animation_state(wv, e, AnimationType::IDLE);
    return;
  }

  const auto& enemyPos = wv->read<PositionComponent>(*optNearestEnemy);

  // Kite directly away from the enemy
  auto to_enemy = enemyPos.map_position - pos.map_position;
  auto to_enemy_len = glm::length(to_enemy);
  auto to_enemy_dir = to_enemy / glm::max(to_enemy_len, 0.001f);
  if (to_enemy_len < 0.01f) {
    to_enemy_len = 0.01f;
    to_enemy_dir = glm::vec2{0.01f, 0.f};
  }

  orientation.radAngle = glm::atan(to_enemy_dir.x, to_enemy_dir.y);
  pos.map_position += -to_enemy_dir * dt * kKiteAwaySpeed;
  maybe_set_animation_state(
      wv, e, AnimationType::RUN);  // TODO (sessamekesh): Play it backwards!

  wv->attach_or_replace<ProjectileFiringIntent>(
      e, ProjectileFiringIntent{enemyPos.map_position});
}

static void spray_n_pray(igecs::WorldView* wv, entt::entity e, float dt,
                         std::uint32_t rngBase,
                         const CtxSpatialIndex& spatial_index,
                         PositionComponent& pos,
                         OrientationComponent& orientation) {
  auto& target = wv->attach_or_replace<SprayNPrayTarget>(
      e, SprayNPrayTarget{
             .currentTarget = {0.f, 0.f},
             .timeToSwitch = 0.f,
             .rngOffset = rngBase + 368u,
         });

  target.timeToSwitch -= dt;
  if (target.timeToSwitch <= 0.f) {
    std::random_device rd;
    std::mt19937 gen(rd());
    gen.seed(target.rngOffset);
    std::uniform_real_distribution<> angle_distribution(0.f, glm::pi<float>());
    float angle = angle_distribution(gen);

    glm::vec2 offset = glm::vec2(glm::sin(angle), glm::cos(angle));
    target.rngOffset++;
    target.currentTarget = pos.map_position + offset * 10.f;
    target.timeToSwitch = kSpraySwitchTime;

    orientation.radAngle = glm::atan(offset.x, offset.y);
    wv->attach_or_replace<ProjectileFiringIntent>(
        e, ProjectileFiringIntent{target.currentTarget});
  }

  maybe_set_animation_state(wv, e, AnimationType::IDLE);
}

void HeroLocomotionSystem::run(igecs::WorldView* wv) {
  const auto& ctxSpatialIndex = wv->ctx<CtxSpatialIndex>();
  const auto& dt = wv->ctx<CtxFrameTime>().secondsSinceLastFrame;
  auto view = wv->view<const HeroStrategyComponent, PositionComponent,
                       OrientationComponent, const HeroTag>();

  for (auto [e, strategy, pos, orientation] : view.each()) {
    switch (strategy.strategy) {
      case HeroStrategy::KiteForDays:
        kite(wv, e, dt, ctxSpatialIndex, pos, orientation);
        break;
      case HeroStrategy::SprayNPray:
      default:
        spray_n_pray(wv, e, dt, strategy.rngSeed, ctxSpatialIndex, pos,
                     orientation);
        break;
    }
  }
}

}  // namespace igdemo
