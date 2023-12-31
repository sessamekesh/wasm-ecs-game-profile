#ifndef IGDEMO_LOGIC_ENEMY_H
#define IGDEMO_LOGIC_ENEMY_H

#include <igdemo/logic/enemy-strategy.h>
#include <igdemo/logic/renderable.h>
#include <igecs/world_view.h>

#include <glm/glm.hpp>

namespace igdemo::enemy {

struct EnemyTag {};

struct EnemyAggro {
  entt::entity e;
};

entt::entity create_enemy_entity(igecs::WorldView* wv,
                                 EnemyStrategy enemyStrategy,
                                 std::uint32_t rngSeed, glm::vec2 startPos,
                                 float startOrientation = 0.f,
                                 ModelType modelType = ModelType::YBOT,
                                 float modelScale = 1.f);

}  // namespace igdemo::enemy

#endif
