#include <igpack-gen/assimp-animation-processor.h>
#include <igpack-gen/assimp-util.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

#include <iostream>
#include <queue>

namespace igpackgen {

bool AssimpAnimationProcessor::export_skeleton(
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
    const IgpackGen::AssimpExtractOzzSkeleton& action,
    const std::string& igasset_name, const std::string& assimp_file_raw) {
  const auto& scene = load_scene(assimp_file_raw, igasset_name);
  if (!scene) {
    std::cerr << "Could not load scene for " << igasset_name << std::endl;
    return false;
  }

  if (!action.skeleton_root_node_name()) {
    std::cerr << "No skeleton root provided - cannot extract skeleton for "
              << igasset_name << std::endl;
    return false;
  }

  //
  // Step 1: Find the root node of the scene referenced by the input action
  const aiNode* root_node = scene->scene->mRootNode->FindNode(
      action.skeleton_root_node_name()->c_str());
  if (!root_node) {
    std::cerr << "Failed to find root node "
              << action.skeleton_root_node_name()->str() << " in asset "
              << igasset_name << ", aborting skeleton extraction" << std::endl;
    return false;
  }

  //
  // Step 2: Build the cumulative root transformation, so that the data fed to
  //  to Ozz is correct relative to the skeleton root instead of the file root.
  aiMatrix4x4 root_transform = root_node->mTransformation;
  for (const aiNode* n = root_node->mParent; n != nullptr; n = n->mParent) {
    root_transform = n->mTransformation * root_transform;
  }

  //
  // Step 3: Traverse the Assimp scene graph and add all child nodes to the
  //  skeleton builder.
  struct ConstructionNode {
    const aiNode* node;
    ozz::animation::offline::RawSkeleton::Joint& joint;
    aiMatrix4x4 transform;
  };

  ozz::animation::offline::RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(1);

  std::queue<ConstructionNode> remaining_nodes;
  remaining_nodes.push({root_node, raw_skeleton.roots[0], root_transform});

  while (!remaining_nodes.empty()) {
    ConstructionNode next = remaining_nodes.front();
    remaining_nodes.pop();

    next.joint.name = ozz::string(next.node->mName.C_Str());

    aiVector3D position, scale;
    aiQuaternion rotation;
    next.transform.Decompose(scale, rotation, position);
    next.joint.transform.translation =
        ozz::math::Float3(position.x, position.y, position.z);
    next.joint.transform.rotation =
        ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
    next.joint.transform.scale = ozz::math::Float3(scale.x, scale.y, scale.z);

    // Do in a separate for loop to avoid resizing and invalidating references
    for (int i = 0; i < next.node->mNumChildren; i++) {
      next.joint.children.push_back(
          ozz::animation::offline::RawSkeleton::Joint{});
    }

    for (int i = 0; i < next.node->mNumChildren; i++) {
      const aiNode* next_node = next.node->mChildren[i];
      remaining_nodes.push(
          {next_node, next.joint.children[i], next_node->mTransformation});
    }
  }

  //
  // Step 4: Construct the skeleton using the Ozz offline library
  if (!raw_skeleton.Validate()) {
    std::cerr << "Raw skeleton validation failed for " << igasset_name
              << std::endl;
    return false;
  }

  ozz::animation::offline::SkeletonBuilder skeleton_builder;
  auto skeleton = skeleton_builder(raw_skeleton);
  if (skeleton == nullptr) {
    std::cerr << "Failed to construct raw skeleton for " << igasset_name
              << std::endl;
    return false;
  }

  ozz::io::MemoryStream mem_stream;
  ozz::io::OArchive archive(&mem_stream);
  archive << *skeleton;

  //
  // Step 5: Serialize to output
  mem_stream.Seek(0, ozz::io::MemoryStream::kSet);
  std::string raw_ozz(mem_stream.Size(), '\0');
  mem_stream.Read(reinterpret_cast<void*>(&raw_ozz[0]), mem_stream.Size());

  auto fb_ozz_bin = fbb.CreateVector(
      reinterpret_cast<const std::uint8_t*>(raw_ozz.data()), raw_ozz.size());
  auto fb_skeleton = IgAsset::CreateOzzSkeleton(fbb, fb_ozz_bin);
  auto fb_name = fbb.CreateString(igasset_name);
  auto fb_single_asset = IgAsset::CreateSingleAsset(
      fbb, fb_name, IgAsset::SingleAssetData_OzzSkeleton, fb_skeleton.Union());

  asset_list.push_back(fb_single_asset);

  return true;
}

bool AssimpAnimationProcessor::export_animation(
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
    const IgpackGen::AssimpExtractOzzAnimation& action,
    const std::string& igasset_name, const std::string& assimp_file_raw) {
  //
  // Step 1: Find the appropriate Assimp animation in the input file
  const auto& scene = load_scene(assimp_file_raw, igasset_name);
  if (!scene) {
    std::cerr << "Could not load scene for " << igasset_name << std::endl;
    return false;
  }

  aiAnimation* animation = nullptr;
  for (int i = 0; i < scene->scene->mNumAnimations; i++) {
    if (action.animation_name()->str() ==
        scene->scene->mAnimations[i]->mName.C_Str()) {
      animation = scene->scene->mAnimations[i];
      break;
    }
  }

  if (animation == nullptr) {
    std::cerr << "Failed to load animation " << action.animation_name()->str()
              << " for " << igasset_name << std::endl;
    return false;
  }

  //
  // Step 2: Parse out metadata and prepare an Ozz RawAnimation object
  double ticks = animation->mDuration;
  double ticks_per_second = animation->mTicksPerSecond;
  if (ticks_per_second < 0.000001) {
    ticks_per_second = 1.;
  }
  ozz::animation::offline::RawAnimation raw_animation;
  raw_animation.duration = (float)(ticks / ticks_per_second);
  raw_animation.name = action.animation_name()->str();
  raw_animation.tracks.resize(animation->mNumChannels);

  std::vector<std::string> bone_names;
  bone_names.reserve(animation->mNumChannels);

  for (int channel_idx = 0; channel_idx < animation->mNumChannels;
       channel_idx++) {
    const aiNodeAnim* channel = animation->mChannels[channel_idx];

    std::string bone_name = channel->mNodeName.C_Str();
    bone_names.push_back(bone_name);

    int bone_idx = channel_idx;
    for (int pos_key_idx = 0; pos_key_idx < channel->mNumPositionKeys;
         pos_key_idx++) {
      const aiVectorKey& pos_key = channel->mPositionKeys[pos_key_idx];
      double t = pos_key.mTime / animation->mTicksPerSecond;
      const aiVector3D& v = pos_key.mValue;

      raw_animation.tracks[bone_idx].translations.push_back(
          {(float)(t), ozz::math::Float3(v.x, v.y, v.z)});
    }

    for (int rot_key_idx = 0; rot_key_idx < channel->mNumRotationKeys;
         rot_key_idx++) {
      const aiQuatKey& rot_key = channel->mRotationKeys[rot_key_idx];
      double t = rot_key.mTime / animation->mTicksPerSecond;
      const aiQuaternion& q = rot_key.mValue;

      raw_animation.tracks[bone_idx].rotations.push_back(
          {(float)(t), ozz::math::Quaternion(q.x, q.y, q.z, q.w)});
    }

    for (int scl_key_idx = 0; scl_key_idx < channel->mNumScalingKeys;
         scl_key_idx++) {
      const aiVectorKey& scl_key = channel->mScalingKeys[scl_key_idx];
      double t = scl_key.mTime / animation->mTicksPerSecond;
      const aiVector3D& s = scl_key.mValue;

      raw_animation.tracks[bone_idx].scales.push_back(
          {(float)(t), ozz::math::Float3(s.x, s.y, s.z)});
    }
  }

  //
  // Step 3: Validate and optimize Ozz animation object
  if (!raw_animation.Validate()) {
    std::cerr << "Validation failed for ozz raw animation " << igasset_name
              << std::endl;
    return false;
  }

  // TODO (sessamekesh): Play around with animation optimization here!

  //
  // Step 4: Serialize and output Ozz runtime animation object
  ozz::animation::offline::AnimationBuilder builder;
  auto runtime_animation = builder(raw_animation);
  if (runtime_animation == nullptr) {
    std::cerr << "Failed to generate animation " << igasset_name << std::endl;
    return false;
  }

  ozz::io::MemoryStream mem_stream;
  ozz::io::OArchive archive(&mem_stream);
  archive << *runtime_animation;

  //
  // Step 5: Serialize to output
  mem_stream.Seek(0, ozz::io::MemoryStream::kSet);
  std::string raw_ozz(mem_stream.Size(), '\0');
  mem_stream.Read(reinterpret_cast<void*>(&raw_ozz[0]), mem_stream.Size());

  std::vector<flatbuffers::Offset<flatbuffers::String>> fb_bone_names_vec;
  for (int i = 0; i < bone_names.size(); i++) {
    fb_bone_names_vec.push_back(fbb.CreateString(bone_names[i]));
  }
  auto fb_bone_names = fbb.CreateVector(fb_bone_names_vec);

  auto fb_ozz_bin = fbb.CreateVector(
      reinterpret_cast<const std::uint8_t*>(raw_ozz.data()), raw_ozz.size());
  auto fb_skeleton =
      IgAsset::CreateOzzAnimation(fbb, fb_ozz_bin, fb_bone_names);
  auto fb_name = fbb.CreateString(igasset_name);
  auto fb_single_asset = IgAsset::CreateSingleAsset(
      fbb, fb_name, IgAsset::SingleAssetData_OzzAnimation, fb_skeleton.Union());

  asset_list.push_back(fb_single_asset);

  return true;
}

}  // namespace igpackgen