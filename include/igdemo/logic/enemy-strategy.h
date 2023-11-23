#ifndef IGDEMO_LOGIC_ENEMY_STRATEGY_H
#define IGDEMO_LOGIC_ENEMY_STRATEGY_H

#include <cstdint>

namespace igdemo {

enum class EnemyStrategy {
  WanderLikeAChuckleFuck,
  BlitzNearestHero,
  RespondIfProvoked,
};

struct EnemyStrategyComponent {
  EnemyStrategy strategy;
  std::uint32_t rngSeed;
};

}  // namespace igdemo

#endif
