#include <igdemo/logic/combat.h>
#include <igdemo/logic/hero.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/renderable.h>

namespace igdemo {

entt::entity create_hero_entity(igecs::WorldView* wv, HeroStrategy heroStrategy,
                                std::uint32_t rngSeed, glm::vec2 startPos,
                                float startOrientation, ModelType modelType,
                                float modelScale) {
  auto e = wv->create();

  wv->attach<PositionComponent>(e, PositionComponent{startPos});
  wv->attach<OrientationComponent>(e, OrientationComponent{startOrientation});
  wv->attach<HealthComponent>(e, HealthComponent{2500.f, 2500.f});
  wv->attach<RenderableComponent>(
      e, RenderableComponent{modelType, MaterialType::GREEN,
                             AnimationType::IDLE, modelScale});
  wv->attach<HeroTag>(e);
  wv->attach<HeroStrategyComponent>(
      e, HeroStrategyComponent{heroStrategy, rngSeed});

  return e;
}

}  // namespace igdemo
