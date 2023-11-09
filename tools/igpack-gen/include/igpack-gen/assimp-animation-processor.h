#ifndef IGPACK_GEN_ASSIMP_ANIMATION_PROCESSOR_H
#define IGPACK_GEN_ASSIMP_ANIMATION_PROCESSOR_H

#include <igasset/schema/igasset.h>
#include <igpack-gen/schema/igpack-plan.h>

namespace igpackgen {

class AssimpAnimationProcessor {
 public:
  bool export_skeleton(
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
      const IgpackGen::AssimpExtractOzzSkeleton& action,
      const std::string& igasset_name, const std::string& assimp_file_raw);
  bool export_animation(
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
      const IgpackGen::AssimpExtractOzzAnimation& action,
      const std::string& igasset_name, const std::string& assimp_file_raw);
};

}  // namespace igpackgen

#endif
