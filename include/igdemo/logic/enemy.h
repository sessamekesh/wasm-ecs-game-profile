#ifndef IGDEMO_LOGIC_ENEMY_H
#define IGDEMO_LOGIC_ENEMY_H

#include <igdemo/logic/renderable.h>
#include <igecs/world_view.h>

#include <glm/glm.hpp>

namespace igdemo::enemy {

struct EnemyTag {};

entt::entity create_enemy_entity(igecs::WorldView* wv, glm::vec2 startPos,
                                 float startOrientation = 0.f,
                                 ModelType modelType = ModelType::YBOT,
                                 float modelScale = 1.f);

}  // namespace igdemo::enemy

#endif
