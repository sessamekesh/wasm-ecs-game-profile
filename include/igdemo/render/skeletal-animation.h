#ifndef IGDEMO_RENDER_SKELETAL_ANIMATION_H
#define IGDEMO_RENDER_SKELETAL_ANIMATION_H

#include <igasset/ozz_jobs.h>
#include <igasync/promise.h>
#include <igecs/profile/frame_profiler.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

namespace igdemo {

/**
 * @brief Component that tracks animation state for an entity
 */
struct AnimationStateComponent {
  igasset::OzzAnimationWithNames* animation;
  float sample_time;
  bool loop;
};

struct GpuAnimationBufferComponent {
  std::vector<glm::mat4> gpuSkinBuffer;
};

}  // namespace igdemo

#endif
