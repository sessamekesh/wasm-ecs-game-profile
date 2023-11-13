#ifndef IGDEMO_LOGIC_RENDERABLE_H
#define IGDEMO_LOGIC_RENDERABLE_H

namespace igdemo {

enum class ModelType {
  YBOT,
};

enum class MaterialType {
  RED,
  GREEN,
  BLUE,
};

enum class AnimationType {
  WALK,
  RUN,
  IDLE,
  DEFEATED,
};

struct RenderableComponent {
  ModelType type;
  MaterialType material;
  AnimationType animation;
  float scale;
};

}  // namespace igdemo

#endif
