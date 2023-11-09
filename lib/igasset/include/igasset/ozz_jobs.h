#ifndef IGASSET_OZZ_JOBS_H
#define IGASSET_OZZ_JOBS_H

#include <igasset/ozz_wrappers.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/soa_float4x4.h>

#include <glm/glm.hpp>

namespace igasset {

struct RemapAnimationToSkeletonIndicesJob {
  class Context {
   public:
    Context();

    bool check_or_init(ozz::animation::Skeleton* skeleton,
                       OzzAnimationWithNames* animation);
    const std::vector<int32_t>& index_remappings() const {
      return index_remappings_;
    }

   private:
    std::vector<int32_t> index_remappings_;
    ozz::animation::Skeleton* skeleton_;
    OzzAnimationWithNames* animation_;
  };

  RemapAnimationToSkeletonIndicesJob();

  bool Validate() const;

  bool Run() const;

  ozz::animation::Skeleton* skeleton;
  OzzAnimationWithNames* animation;
  Context* context;
  ozz::span<ozz::math::SoaTransform> input;
  ozz::span<ozz::math::SoaTransform> output;
};

struct PrepareGpuSkinningDataJob {
  class Context {
   public:
    Context();
    bool check_or_init(ozz::animation::Skeleton* skeleton,
                       std::vector<std::string>* model_bones);
    const std::vector<int32_t>& index_remappings() const {
      return index_remappings_;
    }

   private:
    ozz::animation::Skeleton* skeleton_;
    std::vector<std::string>* model_bones_;
    std::string* begin_;
    std::string* end_;
    std::vector<int32_t> index_remappings_;
  };

  PrepareGpuSkinningDataJob();

  bool Validate() const;
  bool Run() const;

  ozz::animation::Skeleton* skeleton;
  std::vector<std::string>* model_bones;
  std::vector<glm::mat4>* inv_bind_poses;
  Context* context;
  ozz::span<ozz::math::Float4x4> model_space_input;
  ozz::span<glm::mat4> output;
};

}  // namespace igasset

#endif
