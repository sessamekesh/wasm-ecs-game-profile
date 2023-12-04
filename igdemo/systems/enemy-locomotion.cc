#include <igdemo/logic/enemy.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/logic/levelmetadata.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/projectile.h>
#include <igdemo/logic/renderable.h>
#include <igdemo/logic/spatial-index.h>
#include <igdemo/systems/enemy-locomotion.h>

#include <glm/gtx/norm.hpp>
#include <random>

namespace {

const float kEnemyProvokedMovementSpeed = 7.4f;
const float kProvokedProjectileCd = 0.35f;
const float kProvokedProjectileSpeed = 0.15f;

const float kChucklefuckWanderSpeed = 1.5f;

struct ProjectileFireCooldown {
  float timeRemaining;
};

struct NextChucklefuckWanderLocation {
  int rng_seed;
  int location_rng_offset;
  glm::vec2 map_position;
};

}  // namespace

namespace igdemo {

const igecs::WorldView::Decl& UpdateEnemiesSystem::decl() {
  static igecs::WorldView::Decl d =
      igecs::WorldView::Decl()
          .ctx_reads<CtxSpatialIndex>()
          .ctx_reads<CtxFrameTime>()
          .ctx_reads<CtxLevelMetadata>()
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
    renderable.animation = animation_type;
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
  orientation.radAngle = glm::atan(to_enemy_dir.x, to_enemy_dir.y);
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

static void enemy_wander(igecs::WorldView* wv, entt::entity e, float dt,
                         std::uint32_t rngBase, PositionComponent& pos,
                         OrientationComponent& orientation) {
  const auto& ctxLvlMetadata = wv->ctx<CtxLevelMetadata>();

  if (!wv->has<NextChucklefuckWanderLocation>(e)) {
    NextChucklefuckWanderLocation wl{};
    // TODO (sessamekesh): Add another offset here based on entity ID or
    // something?
    wl.rng_seed = rngBase + 1234;
    wl.location_rng_offset = 0;
    wl.map_position = pos.map_position;
    wv->attach<NextChucklefuckWanderLocation>(e, wl);
  }

  auto& wander_location = wv->write<NextChucklefuckWanderLocation>(e);

  float distance = kChucklefuckWanderSpeed * dt;
  glm::vec2 toDest = wander_location.map_position - pos.map_position;
  float distToDest = glm::length(toDest);
  glm::vec2 dirToDest = toDest / glm::max(distToDest, 0.001f);

  while (distToDest <= distance) {
    distance -= distToDest;
    pos.map_position = wander_location.map_position;

    std::random_device rd;
    std::mt19937 gen(rd());
    gen.seed(wander_location.rng_seed + wander_location.location_rng_offset);
    std::uniform_real_distribution<> x_pos_distribution(
        ctxLvlMetadata.mapXMin,
        ctxLvlMetadata.mapXMin + ctxLvlMetadata.mapXRange);
    std::uniform_real_distribution<> z_pos_distribution(
        ctxLvlMetadata.mapZMin,
        ctxLvlMetadata.mapZMin + ctxLvlMetadata.mapZRange);

    wander_location.location_rng_offset++;
    wander_location.map_position.x = x_pos_distribution(gen);
    wander_location.map_position.y = x_pos_distribution(gen);

    toDest = wander_location.map_position - pos.map_position;
    distToDest = glm::length(toDest);
    dirToDest = toDest / glm::max(distToDest, 0.001f);
  }

  pos.map_position += dirToDest * distance;
  orientation.radAngle = glm::atan(dirToDest.x, dirToDest.y);
  maybe_set_animation_state(wv, e, AnimationType::WALK);
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
        enemy_wander(wv, e, dt, strategy.rngSeed, pos, orientation);
    }
  }
}

}  // namespace igdemo
