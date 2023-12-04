#ifndef IGDEMO_LOGIC_PROJECTILE_H
#define IGDEMO_LOGIC_PROJECTILE_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace igdemo {

enum class ProjectileSource {
  Hero,
  Enemy,
};

struct ProjectileFiringIntent {
  glm::vec2 target;
};

struct ProjectileFireCooldown {
  // What is the maximum number of stored projectiles?
  int maxAllowed;
  // How many are currently stored?
  int currentStored;
  // What is the main cooldown (how long between getting stored projectiles?)
  float mainCd;
  // What is the secondary (how long between firing stored projectiles?)
  float secondaryCd;

  // Remaining cooldowns (tick down)
  float mainCdRemaining;
  float secondaryCdRemaining;
};

struct Projectile {
  ProjectileSource type;
  entt::entity source;
  glm::vec2 pos;
  glm::vec2 velocity;
};

}  // namespace igdemo

#endif
