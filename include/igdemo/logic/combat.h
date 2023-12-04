#ifndef IGDEMO_LOGIC_COMBAT_H
#define IGDEMO_LOGIC_COMBAT_H

namespace igdemo {

struct HealthComponent {
  float maxHealth;
  float currentHealth;
};

struct LifetimeComponent {
  float timeRemaining;
};

}  // namespace igdemo

#endif
