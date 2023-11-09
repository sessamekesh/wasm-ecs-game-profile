#ifndef IGPACK_GEN_ASSIMP_GEO_PROCESSOR_H
#define IGPACK_GEN_ASSIMP_GEO_PROCESSOR_H

#include <igasset/schema/igasset.h>
#include <igpack-gen/schema/igpack-plan.h>

namespace igpackgen {

class AssimpGeoProcessor {
 public:
  bool export_draco_geo(
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
      const IgpackGen::AssimpToDracoAction& action,
      const std::string& igasset_name, const std::string& assimp_file_raw);
};

}  // namespace igpackgen

#endif
