#include <igasset/ozz_jobs.h>
#include <ozz/base/maths/quaternion.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/transform.h>
#include <ozz/base/maths/vec_float.h>

namespace igasset {

// TODO (sessamekesh): Grab this with large inspiration from
//   ozz::runtime::SamplingJob
// Use context to cache skeleton->animation remapping, so it's only computed
//   once

RemapAnimationToSkeletonIndicesJob::Context::Context()
    : skeleton_(nullptr), animation_(nullptr) {}

bool RemapAnimationToSkeletonIndicesJob::Context::check_or_init(
    ozz::animation::Skeleton* skeleton, OzzAnimationWithNames* animation) {
  if (skeleton == nullptr || animation == nullptr) {
    return false;
  }
  if (skeleton == skeleton_ && animation == animation_) {
    return true;
  }

  skeleton_ = skeleton;
  animation_ = animation;

  auto skeleton_names = skeleton_->joint_names();
  const auto& animation_names = animation_->channel_bone_names;

  index_remappings_.resize(animation_names.size(), -1);
  for (int i = 0; i < animation_names.size(); i++) {
    for (int j = 0; j < skeleton_names.size(); j++) {
      const auto* skeleton_name = skeleton_names[j];
      const std::string& animation_name = animation_names[i];
      if (animation_name == skeleton_name) {
        index_remappings_[i] = j;
        break;
      }
    }
  }

  return true;
}

RemapAnimationToSkeletonIndicesJob::RemapAnimationToSkeletonIndicesJob()
    : skeleton(nullptr), animation(nullptr), context(nullptr) {}

bool RemapAnimationToSkeletonIndicesJob::Validate() const {
  if (skeleton == nullptr || animation == nullptr || context == nullptr ||
      input.empty() || output.empty()) {
    return false;
  }

  return context->check_or_init(skeleton, animation);
}

bool RemapAnimationToSkeletonIndicesJob::Run() const {
  if (!Validate()) {
    return false;
  }

  const auto& remappings = context->index_remappings();
  // Unexpected: joint_rest_poses is too small here?
  auto rest_poses = skeleton->joint_rest_poses();
  auto num_soas = skeleton->num_soa_joints();
  auto num_elements = skeleton->joint_names().size();

  for (int i = 0; i < num_soas; i++) {
    // Load in default (in SOA space of course)!
    // output[i] = skeleton->joint_rest_poses()[i];
    output[i] = ozz::math::SoaTransform::identity();
  }

  // Apply remappings, but again in SOAs!
  for (int i = 0; i < remappings.size(); i++) {
    if (remappings[i] == -1) {
      return false;
    }

    int anim_idx = i;
    int skeleton_idx = remappings[i];

    int anim_soa_idx = anim_idx / 4;
    int anim_soa_offset = anim_idx % 4;
    int skele_soa_idx = skeleton_idx / 4;
    int skele_soa_offset = skeleton_idx % 4;

    const ozz::math::SoaTransform& anim_soa_transform = input[anim_soa_idx];
    ozz::math::SoaTransform& skele_soa_transform = output[skele_soa_idx];

    const float* itx =
        ((float*)&anim_soa_transform.translation.x + anim_soa_offset);
    const float* ity =
        ((float*)&anim_soa_transform.translation.y + anim_soa_offset);
    const float* itz =
        ((float*)&anim_soa_transform.translation.z + anim_soa_offset);

    const float* isx = ((float*)&anim_soa_transform.scale.x + anim_soa_offset);
    const float* isy = ((float*)&anim_soa_transform.scale.y + anim_soa_offset);
    const float* isz = ((float*)&anim_soa_transform.scale.z + anim_soa_offset);

    const float* irx =
        ((float*)&anim_soa_transform.rotation.x + anim_soa_offset);
    const float* iry =
        ((float*)&anim_soa_transform.rotation.y + anim_soa_offset);
    const float* irz =
        ((float*)&anim_soa_transform.rotation.z + anim_soa_offset);
    const float* irw =
        ((float*)&anim_soa_transform.rotation.w + anim_soa_offset);

    float* tx = ((float*)&skele_soa_transform.translation.x + skele_soa_offset);
    float* ty = ((float*)&skele_soa_transform.translation.y + skele_soa_offset);
    float* tz = ((float*)&skele_soa_transform.translation.z + skele_soa_offset);

    float* sx = ((float*)&skele_soa_transform.scale.x + skele_soa_offset);
    float* sy = ((float*)&skele_soa_transform.scale.y + skele_soa_offset);
    float* sz = ((float*)&skele_soa_transform.scale.z + skele_soa_offset);

    float* rx = ((float*)&skele_soa_transform.rotation.x + skele_soa_offset);
    float* ry = ((float*)&skele_soa_transform.rotation.y + skele_soa_offset);
    float* rz = ((float*)&skele_soa_transform.rotation.z + skele_soa_offset);
    float* rw = ((float*)&skele_soa_transform.rotation.w + skele_soa_offset);

    *tx = *itx;
    *ty = *ity;
    *tz = *itz;

    *sx = *isx;
    *sy = *isy;
    *sz = *isz;

    *rx = *irx;
    *ry = *iry;
    *rz = *irz;
    *rw = *irw;
  }

  return true;
}

PrepareGpuSkinningDataJob::Context::Context()
    : skeleton_(nullptr),
      model_bones_(nullptr),
      begin_(nullptr),
      end_(nullptr) {}

bool PrepareGpuSkinningDataJob::Context::check_or_init(
    ozz::animation::Skeleton* skeleton, std::vector<std::string>* model_bones) {
  if (skeleton == nullptr || model_bones == nullptr) {
    return false;
  }

  const auto& vec = *model_bones;
  if (skeleton == skeleton_ && model_bones_ == model_bones &&
      &vec[0] == begin_ && &vec[vec.size() - 1] == end_) {
    return true;
  }

  skeleton_ = skeleton;
  model_bones_ = model_bones;
  begin_ = &model_bones_->at(0);
  end_ = &model_bones_->at(model_bones_->size() - 1);

  index_remappings_.resize(vec.size(), -1);
  for (int model_idx = 0; model_idx < vec.size(); model_idx++) {
    for (int skele_idx = 0; skele_idx < skeleton->joint_names().size();
         skele_idx++) {
      if (vec[model_idx] == skeleton->joint_names()[skele_idx]) {
        index_remappings_[model_idx] = skele_idx;
      }
    }

    if (index_remappings_[model_idx] == -1) {
      return false;
    }
  }

  return true;
}

PrepareGpuSkinningDataJob::PrepareGpuSkinningDataJob()
    : skeleton(nullptr),
      model_bones(nullptr),
      inv_bind_poses(nullptr),
      context(nullptr) {}

bool PrepareGpuSkinningDataJob::Validate() const {
  if (skeleton == nullptr || model_bones == nullptr ||
      inv_bind_poses == nullptr || context == nullptr ||
      model_space_input.empty() || output.empty() ||
      model_bones->size() != inv_bind_poses->size()) {
    return false;
  }

  if (!context->check_or_init(skeleton, model_bones)) {
    return false;
  }

  return true;
}

bool PrepareGpuSkinningDataJob::Run() const {
  if (!Validate()) {
    return false;
  }

  const auto& idx_remapping = context->index_remappings();

  for (int model_idx = 0; model_idx < model_bones->size(); model_idx++) {
    const auto& ozz_model = model_space_input[idx_remapping[model_idx]];
    const glm::mat4& inv_bind_pos = inv_bind_poses->at(model_idx);
    glm::mat4 model(((float*)&ozz_model)[0], ((float*)&ozz_model)[1],
                    ((float*)&ozz_model)[2], ((float*)&ozz_model)[3],
                    ((float*)&ozz_model)[4], ((float*)&ozz_model)[5],
                    ((float*)&ozz_model)[6], ((float*)&ozz_model)[7],
                    ((float*)&ozz_model)[8], ((float*)&ozz_model)[9],
                    ((float*)&ozz_model)[10], ((float*)&ozz_model)[11],
                    ((float*)&ozz_model)[12], ((float*)&ozz_model)[13],
                    ((float*)&ozz_model)[14], ((float*)&ozz_model)[15]);

    output[model_idx] = model * inv_bind_pos;
  }

  return true;
}

}  // namespace igasset
