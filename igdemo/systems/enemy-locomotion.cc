#include <igdemo/logic/enemy.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/projectile.h>
#include <igdemo/logic/renderable.h>
#include <igdemo/logic/spatial-index.h>
#include <igdemo/systems/enemy-locomotion.h>

#include <glm/gtx/norm.hpp>

namespace {
const float kEnemyProvokedMovementSpeed = 3.f;
const float kProvokedProjectileCd = 0.35f;
const float kProvokedProjectileSpeed = 0.15f;

struct ProjectileFireCooldown {
  float timeRemaining;
};

struct NextChucklefuckWanderLocation {
  int rng_seed;
  int enemy_offset;
  int location_offset;

  glm::vec2 map_position;
  float orientation;
};

}  // namespace

namespace igdemo {

const igecs::WorldView::Decl& UpdateEnemiesSystem::decl() {
  static igecs::WorldView::Decl d =
      igecs::WorldView::Decl()
          .ctx_reads<CtxSpatialIndex>()
          .ctx_reads<CtxFrameTime>()
          .evt_writes<EvtSpawnProjectile>()
          .reads<enemy::EnemyTag>()
          .reads<enemy::EnemyAggro>()
          .reads<EnemyStrategyComponent>()
          .writes<RenderableComponent>()
          .writes<PositionComponent>()
          .writes<OrientationComponent>()
          .writes<ProjectileFireCooldown>()
          .writes<NextChucklefuckWanderLocation>();

  return d;
}

static void maybe_set_animation_state(igecs::WorldView* wv, entt::entity e,
                                      AnimationType animation_type) {
  if (wv->has<RenderableComponent>(e)) {
    auto& renderable = wv->write<RenderableComponent>(e);
    renderable.animation = AnimationType::IDLE;
  }
}

static void enemy_respond_if_provoked(igecs::WorldView* wv, entt::entity e,
                                      float dt, PositionComponent& pos,
                                      OrientationComponent& orientation) {
  if (!wv->has<enemy::EnemyAggro>(e)) {
    // Just... sit around.
    maybe_set_animation_state(wv, e, AnimationType::IDLE);
    return;
  }

  const auto& aggro = wv->read<enemy::EnemyAggro>(e);
  if (!wv->valid(aggro.e)) {
    wv->remove<enemy::EnemyAggro>(e);
    maybe_set_animation_state(wv, e, AnimationType::IDLE);
    return;
  }

  const auto& enemy_position =
      wv->read<PositionComponent>(aggro.e).map_position;

  auto to_enemy = enemy_position - pos.map_position;
  auto to_enemy_len = glm::length(to_enemy);

  auto to_enemy_dir = to_enemy / glm::max(to_enemy_len, 0.001f);

  // Approach hero
  orientation.radAngle = glm::atan(to_enemy_dir.y, to_enemy_dir.x);
  pos.map_position +=
      to_enemy_dir * glm::min(dt * kEnemyProvokedMovementSpeed, to_enemy_len);
  maybe_set_animation_state(wv, e, AnimationType::RUN);

  // Fire projectile
  if (!wv->has<ProjectileFireCooldown>(e)) {
    wv->attach<ProjectileFireCooldown>(e, ProjectileFireCooldown{0.f});
  }
  auto& cooldown = wv->write<ProjectileFireCooldown>(e);
  cooldown.timeRemaining -= dt;
  if (cooldown.timeRemaining < 0.f) {
    cooldown.timeRemaining = kProvokedProjectileCd;
    Projectile projectile{};
    projectile.source = e;
    projectile.pos = pos.map_position;
    projectile.type = ProjectileSource::Enemy;
    projectile.velocity = to_enemy_dir * kProvokedProjectileSpeed;

    wv->enqueue_event(EvtSpawnProjectile{projectile});
  }
}

static void enemy_blitz(igecs::WorldView* wv, entt::entity e,
                        const CtxSpatialIndex& spatial_index,
                        PositionComponent& pos,
                        OrientationComponent& orientation) {}

static void enemy_wander(igecs::WorldView* wv, entt::entity e,
                         PositionComponent& pos,
                         OrientationComponent& orientation) {
  // TODO (sessamekesh): Attach chucklefuck wander entity if not present.
  // TODO (sessamekesh): Get RNG seed and map bounds from a new context value
  // TODO (sessamekesh): Approach the next location - if reached, create new one
}

void UpdateEnemiesSystem::run(igecs::WorldView* wv) {
  const auto& ctxSpatialIndex = wv->ctx<CtxSpatialIndex>();
  auto view = wv->view<const EnemyStrategyComponent, PositionComponent,
                       OrientationComponent, const enemy::EnemyTag>();
  const auto& dt = wv->ctx<CtxFrameTime>().secondsSinceLastFrame;

  for (auto [e, strategy, pos, orientation] : view.each()) {
    switch (strategy.strategy) {
      case EnemyStrategy::RespondIfProvoked:
        enemy_respond_if_provoked(wv, e, dt, pos, orientation);
        break;
      case EnemyStrategy::BlitzNearestHero:
        enemy_blitz(wv, e, ctxSpatialIndex, pos, orientation);
        break;
      case EnemyStrategy::WanderLikeAChuckleFuck:
      default:
        enemy_wander(wv, e, pos, orientation);
    }
  }
}

}  // namespace igdemo
