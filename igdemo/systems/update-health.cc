#include <igdemo/logic/combat.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/systems/destroy-actor.h>
#include <igdemo/systems/update-health.h>

namespace igdemo {

const float kPctRegenPerSecond = 0.015f;

const igecs::WorldView::Decl& UpdateHealthSystem::decl() {
  static igecs::WorldView::Decl d = igecs::WorldView::Decl()
                                        .ctx_reads<CtxFrameTime>()
                                        .writes<HealthComponent>()
                                        .evt_writes<EvtDestroyActor>();

  return d;
}

void UpdateHealthSystem::run(igecs::WorldView* wv) {
  float dt = wv->ctx<CtxFrameTime>().secondsSinceLastFrame;

  auto view = wv->view<HealthComponent>();

  for (auto [e, health] : view.each()) {
    health.currentHealth += health.maxHealth * dt * kPctRegenPerSecond;
    if (health.currentHealth > health.maxHealth) {
      health.currentHealth = health.maxHealth;
    }

    if (health.currentHealth < 0.f) {
      wv->enqueue_event(EvtDestroyActor{e});
    }
  }
}

}  // namespace igdemo
