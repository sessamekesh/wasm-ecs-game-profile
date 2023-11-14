#ifndef IGPACK_GEN_PLAN_EXECUTOR_H
#define IGPACK_GEN_PLAN_EXECUTOR_H

#include <igpack-gen/assimp-animation-processor.h>
#include <igpack-gen/assimp-geo-processor.h>
#include <igpack-gen/hdr-image.h>
#include <igpack-gen/schema/igpack-plan.h>
#include <igpack-gen/wgsl-processor.h>

#include <filesystem>

namespace igpackgen {

struct PlanInvocationDesc {
  const IgpackGen::IgpackGenPlan* Plan;
  std::filesystem::path InputAssetPathRoot;
  std::filesystem::path OutputAssetPathRoot;
};

class PlanExecutor {
 public:
  bool execute_plan(const PlanInvocationDesc& plan);

 private:
  bool process_wgsl(
      const std::filesystem::path& input_path_root,
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
      const IgpackGen::CopyWgslSourceAction* action, std::string igasset_name);
  bool process_assimp_to_draco(
      const std::filesystem::path& input_path_root,
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
      const IgpackGen::AssimpToDracoAction* action, std::string igasset_name);
  bool process_assimp_to_ozz_skeleton(
      const std::filesystem::path& input_path_root,
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
      const IgpackGen::AssimpExtractOzzSkeleton* action,
      std::string igasset_name);
  bool process_assimp_to_ozz_animation(
      const std::filesystem::path& input_path_root,
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
      const IgpackGen::AssimpExtractOzzAnimation* action,
      std::string igasset_name);
  bool process_hdr_image(
      const std::filesystem::path& input_path_root,
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
      const IgpackGen::EncodeRawHdrFile* action, std::string igasset_name);

 private:
  WgslProcessor wgsl_;
  AssimpGeoProcessor assimp_geo_;
  AssimpAnimationProcessor assimp_animation_;
  HdrImageProcessor hdr_image_;
};

}  // namespace igpackgen

#endif
