#include <igasync/promise_combiner.h>
#include <igdemo/logic/combat.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/projectile.h>
#include <igdemo/systems/destroy-actor.h>
#include <igdemo/systems/move-projectile.h>

namespace igdemo {
const igecs::WorldView::Decl& MoveProjectileSystem::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           .ctx_reads<CtxFrameTime>()
                                           .reads<Projectile>()
                                           .writes<LifetimeComponent>()
                                           .writes<PositionComponent>()
                                           .evt_writes<EvtDestroyActor>();

  return decl;
}

static void update_inner(
    igecs::WorldView* wv,
    std::shared_ptr<std::vector<entt::entity>> process_list,
    std::uint32_t startChunk, std::uint32_t ct, float dt) {
  for (std::uint32_t entityIdx = startChunk; entityIdx < startChunk + ct;
       entityIdx++) {
    entt::entity e = (*process_list)[entityIdx];

    auto& pos = wv->write<PositionComponent>(e);
    auto& lifetime = wv->write<LifetimeComponent>(e);
    const auto& projectile = wv->read<Projectile>(e);

    // Step 1: Update the time remaining on the thingy
    lifetime.timeRemaining -= dt;
    if (lifetime.timeRemaining <= 0.f) {
      wv->enqueue_event<EvtDestroyActor>(EvtDestroyActor{e});
      continue;
    }

    // Step 2: update the position
    pos.map_position += projectile.velocity * dt;

    // TODO (sessamekesh): Check for collisions and spawn event if there is one
  }
}

static void update(igecs::WorldView* wv,
                   std::shared_ptr<std::vector<entt::entity>> process_list,
                   std::uint32_t startChunk, std::uint32_t ct, float dt,
                   std::shared_ptr<igasync::Promise<void>> rsl) {
  // Inner method to make it difficult to accidentally not resolve the promise
  update_inner(wv, process_list, startChunk, ct, dt);
  rsl->resolve();
}

std::shared_ptr<igasync::Promise<void>> MoveProjectileSystem::run(
    igecs::WorldView* wv, std::shared_ptr<igasync::TaskList> main_thread,
    std::shared_ptr<igasync::TaskList> any_thread,
    std::function<void(igasync::TaskProfile profile)> profile_cb) {
  auto process_list = std::make_shared<std::vector<entt::entity>>();
  float dt = wv->ctx<CtxFrameTime>().secondsSinceLastFrame;

  // Pass 1 - collect entities that need iteration
  {
    auto view =
        wv->view<PositionComponent, LifetimeComponent, const Projectile>();
    for (auto [e, pos, life, proj] : view.each()) {
      if (wv->valid(e)) {
        process_list->push_back(e);
      } else {
        process_list->push_back(e);
        // HA?
      }
    }
  }

  // Pass 2 - put together tasks
  auto combiner = igasync::PromiseCombiner::Create();

  const std::uint32_t kChunkSize = 20;
  for (std::uint32_t startChunk = 0; startChunk < process_list->size();
       startChunk += kChunkSize) {
    std::uint32_t ct =
        std::min(kChunkSize,
                 static_cast<std::uint32_t>(process_list->size()) - startChunk);

    auto promise = igasync::Promise<void>::Create();
    any_thread->schedule(igasync::Task::WithProfile(
        profile_cb, update, wv, process_list, startChunk, ct, dt, promise));

    combiner->add(promise, any_thread);
  }

  return combiner->combine([](auto) {}, any_thread);
}
}  // namespace igdemo
