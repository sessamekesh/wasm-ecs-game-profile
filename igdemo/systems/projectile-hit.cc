#include <igdemo/logic/combat.h>
#include <igdemo/logic/enemy.h>
#include <igdemo/logic/hero.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/projectile.h>
#include <igdemo/logic/spatial-index.h>
#include <igdemo/systems/destroy-actor.h>
#include <igdemo/systems/projectile-hit.h>

namespace igdemo {

const float kHeroProjectileRadius = 0.7f;
const float kEnemyProjectileRadius = 0.7f;

const float kHeroProjectileDamage = 50.f;
const float kEnemyProjectileDamage = 20.f;

const igecs::WorldView::Decl& ProjectileHitSystem::decl() {
  static igecs::WorldView::Decl d = igecs::WorldView::Decl()
                                        .merge_in_decl(GridIndex::decl())
                                        .ctx_reads<CtxSpatialIndex>()
                                        .reads<Projectile>()
                                        .reads<PositionComponent>()
                                        .writes<HealthComponent>()
                                        .evt_writes<EvtDestroyActor>();

  return d;
}

static void projectile_hit(igecs::WorldView* wv, entt::entity enemy,
                           float hitDamage) {
  if (!wv->has<HealthComponent>(enemy)) {
    return;
  }

  auto& health = wv->write<HealthComponent>(enemy);
  health.currentHealth -= hitDamage;
}

void ProjectileHitSystem::run(igecs::WorldView* wv) {
  const auto& spatial_index = wv->ctx<CtxSpatialIndex>();

  auto view = wv->view<const Projectile, const PositionComponent>();

  std::set<entt::entity> projectiles_to_destroy;

  for (auto [e, projectile, position] : view.each()) {
    if (projectile.type == ProjectileSource::Hero) {
      for (auto collide_entity : spatial_index.enemyIndex.collisions(
               wv, position.map_position, kHeroProjectileRadius)) {
        projectile_hit(wv, collide_entity, kHeroProjectileDamage);
        projectiles_to_destroy.insert(e);
      }
    } else if (projectile.type == ProjectileSource::Enemy) {
      for (auto collide_entity : spatial_index.heroIndex.collisions(
               wv, position.map_position, kEnemyProjectileRadius)) {
        projectile_hit(wv, collide_entity, kEnemyProjectileDamage);
        projectiles_to_destroy.insert(e);
      }
    }
  }

  for (auto e : projectiles_to_destroy) {
    wv->enqueue_event<EvtDestroyActor>(EvtDestroyActor{e});
  }
}

}  // namespace igdemo
