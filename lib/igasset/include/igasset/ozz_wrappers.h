#ifndef IGASSET_OZZ_WRAPPERS_H
#define IGASSET_OZZ_WRAPPERS_H

#include <ozz/animation/runtime/animation.h>

#include <string>
#include <vector>

namespace igasset {

struct OzzAnimationWithNames {
  ozz::animation::Animation animation;
  std::vector<std::string> channel_bone_names;
};

}  // namespace igasset

#endif
