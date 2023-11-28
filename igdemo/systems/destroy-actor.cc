#include <igdemo/logic/enemy.h>
#include <igdemo/logic/hero.h>
#include <igdemo/logic/spatial-index.h>
#include <igdemo/systems/destroy-actor.h>

namespace igdemo {

const igecs::WorldView::Decl& DestroyActorSystem::decl() {
  static igecs::WorldView::Decl d = igecs::WorldView::Decl()
                                        .evt_consumes<EvtDestroyActor>()
                                        .ctx_writes<CtxSpatialIndex>()
                                        .reads<HeroTag>()
                                        .reads<enemy::EnemyTag>();
  return d;
}

void DestroyActorSystem::run(igecs::WorldView* wv) {
  auto events = wv->consume_events<EvtDestroyActor>();

  auto& ctxSpatialIndex = wv->mut_ctx<CtxSpatialIndex>();

  for (auto& evt : events) {
    if (wv->has<HeroTag>(evt.e)) {
      ctxSpatialIndex.heroIndex.remove(wv, evt.e);
    }

    if (wv->has<enemy::EnemyTag>(evt.e)) {
      ctxSpatialIndex.enemyIndex.remove(wv, evt.e);
    }

    wv->destroy(evt.e);
  }
}

}  // namespace igdemo
