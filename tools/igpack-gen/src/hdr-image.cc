#include <igpack-gen/hdr-image.h>

namespace igpackgen {
bool HdrImageProcessor::export_raw_hdr(
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
    const IgpackGen::EncodeRawHdrFile& action, const std::string& igasset_name,
    const std::string& hdr_file_raw) {
  auto fb_hdr_bin = fbb.CreateVector(
      reinterpret_cast<const std::uint8_t*>(hdr_file_raw.data()),
      hdr_file_raw.size());
  auto fb_hdr = IgAsset::CreateHdrRaw(fbb, fb_hdr_bin);
  auto fb_name = fbb.CreateString(igasset_name);
  auto fb_single_asset = IgAsset::CreateSingleAsset(
      fbb, fb_name, IgAsset::SingleAssetData_HdrRaw, fb_hdr.Union());

  asset_list.push_back(fb_single_asset);

  return true;
}
}  // namespace igpackgen
