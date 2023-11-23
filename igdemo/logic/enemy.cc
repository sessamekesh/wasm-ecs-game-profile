#include <igdemo/logic/combat.h>
#include <igdemo/logic/enemy.h>
#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/renderable.h>

namespace igdemo::enemy {

entt::entity create_enemy_entity(igecs::WorldView* wv,
                                 EnemyStrategy enemyStrategy,
                                 std::uint32_t rngSeed, glm::vec2 startPos,
                                 float startOrientation, ModelType modelType,
                                 float modelScale) {
  auto e = wv->create();

  wv->attach<PositionComponent>(e, PositionComponent{startPos});
  wv->attach<OrientationComponent>(e, OrientationComponent{startOrientation});

  wv->attach<HealthComponent>(e, HealthComponent{100.f, 100.f});

  wv->attach<RenderableComponent>(
      e, RenderableComponent{modelType, MaterialType::RED, AnimationType::IDLE,
                             modelScale});
  wv->attach<EnemyTag>(e);
  wv->attach<EnemyStrategyComponent>(
      e, EnemyStrategyComponent{enemyStrategy, rngSeed});

  return e;
}

}  // namespace igdemo::enemy
