#ifndef IGPACK_GEN_WGSL_PROCESSOR_H
#define IGPACK_GEN_WGSL_PROCESSOR_H

#include <igasset/schema/igasset.h>
#include <igpack-gen/schema/igpack-plan.h>

namespace igpackgen {

class WgslProcessor {
 public:
  bool copy_wgsl_source(
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>&
          asset_list,
      const IgpackGen::CopyWgslSourceAction& action,
      const std::string& igasset_name, const std::string& wgsl_source) const;
};

}  // namespace igpackgen

#endif
