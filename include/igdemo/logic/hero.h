#ifndef IGDEMO_LOGIC_HERO_H
#define IGDEMO_LOGIC_HERO_H

#include <igdemo/logic/renderable.h>
#include <igecs/world_view.h>

#include <glm/glm.hpp>

namespace igdemo {

enum class HeroStrategy {
  KiteForDays,
  SprayNPray,
  HoldYourGround,
};

struct HeroStrategyComponent {
  HeroStrategy strategy;
  std::uint32_t rngSeed;
};

struct HeroTag {};

entt::entity create_hero_entity(igecs::WorldView* wv, HeroStrategy heroStrategy,
                                std::uint32_t rngSeed, glm::vec2 startPos,
                                float startOrientation, ModelType modelType,
                                float modelScale = 1.f);

}  // namespace igdemo

#endif
