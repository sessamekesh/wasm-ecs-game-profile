#ifndef IGDEMO_LOGIC_PROJECTILE_H
#define IGDEMO_LOGIC_PROJECTILE_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace igdemo {

enum class ProjectileSource {
  Hero,
  Enemy,
};

struct Projectile {
  ProjectileSource type;
  entt::entity source;
  glm::vec2 pos;
  glm::vec2 velocity;
};

struct EvtSpawnProjectile {
  Projectile projectile;
};

}  // namespace igdemo

#endif
