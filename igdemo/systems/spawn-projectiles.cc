#include <igdemo/logic/combat.h>
#include <igdemo/logic/enemy.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/logic/hero.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/projectile.h>
#include <igdemo/systems/spawn-projectiles.h>

namespace igdemo {

const float kProjectileSpeed = 25.f;

const igecs::WorldView::Decl& SpawnProjectilesSystem::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           .ctx_reads<CtxFrameTime>()
                                           .reads<PositionComponent>()
                                           .reads<ProjectileFiringIntent>()
                                           .reads<HeroTag>()
                                           .reads<enemy::EnemyTag>()
                                           .writes<ProjectileFireCooldown>()
                                           .writes<Projectile>()
                                           .writes<LifetimeComponent>();

  return decl;
}

static void spawn_projectile(igecs::WorldView* wv, entt::entity source,
                             const ProjectileFiringIntent& intent, float dt) {
  if (dt < 0.f) dt = 0.f;

  ProjectileSource source_type;
  if (wv->has<HeroTag>(source)) {
    source_type = ProjectileSource::Hero;
  } else if (wv->has<enemy::EnemyTag>(source)) {
    source_type = ProjectileSource::Enemy;
  } else {
    return;
  }

  if (!wv->has<PositionComponent>(source)) {
    return;
  }

  const auto& pos = wv->read<PositionComponent>(source).map_position;

  auto dir = glm::normalize(intent.target - pos);

  auto e = wv->create();
  wv->attach<LifetimeComponent>(e, 10.f);
  wv->attach<Projectile>(e, Projectile{/* type */
                                       source_type,
                                       /* source */
                                       source,
                                       /* veocity */
                                       dir * kProjectileSpeed});
  wv->attach<PositionComponent>(e, PositionComponent{
                                       pos + dir * kProjectileSpeed * dt,
                                   });
}

void SpawnProjectilesSystem::run(igecs::WorldView* wv) {
  const auto& dt = wv->ctx<CtxFrameTime>().secondsSinceLastFrame;

  auto view = wv->view<const PositionComponent, ProjectileFireCooldown,
                       const ProjectileFiringIntent>();

  for (auto [e, pos, cooldown, intent] : view.each()) {
    // Step 1: update cooldowns
    cooldown.secondaryCdRemaining -= dt;
    cooldown.mainCdRemaining -= dt;

    while (cooldown.mainCdRemaining < 0.f) {
      cooldown.mainCdRemaining += cooldown.mainCd;
      cooldown.currentStored =
          glm::min(cooldown.currentStored + 1, cooldown.maxAllowed);
    }

    while (cooldown.secondaryCdRemaining < 0.f) {
      // Spawn a projectile
      spawn_projectile(wv, e, intent, -cooldown.secondaryCdRemaining);
      cooldown.secondaryCdRemaining += cooldown.secondaryCd;
    }
  }
}

}  // namespace igdemo
