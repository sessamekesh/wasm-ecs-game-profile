#ifndef IGPACK_GEN_HDR_IMAGE_H
#define IGPACK_GEN_HDR_IMAGE_H

#include <igasset/schema/igasset.h>
#include <igpack-gen/schema/igpack-plan.h>

namespace igpackgen {

class HdrImageProcessor {
 public:
  bool export_raw_hdr(
      flatbuffers::FlatBufferBuilder& fbb,
      std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
      const IgpackGen::EncodeRawHdrFile& action,
      const std::string& igasset_name, const std::string& assimp_file_raw);
};

}  // namespace igpackgen

#endif
