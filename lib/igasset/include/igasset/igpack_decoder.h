#ifndef IGASSET_IGPACK_DECODER_H
#define IGASSET_IGPACK_DECODER_H

#include <igasset/draco_dec.h>
#include <igasset/ozz_wrappers.h>
#include <igasset/raw_hdr.h>
#include <igasset/schema/igasset.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

#include <memory>
#include <string>

namespace igasset {

enum class IgpackExtractError {
  AssetExtractError,
  ResourceNotFound,
  WrongResourceType,
  MalformedResourceData,
};

class IgpackDecoder {
 public:
  static std::shared_ptr<IgpackDecoder> Create(std::string data);

  IgpackDecoder(const IgpackDecoder&) = delete;
  IgpackDecoder(IgpackDecoder&&) = default;
  IgpackDecoder& operator=(const IgpackDecoder&) = delete;
  IgpackDecoder& operator=(IgpackDecoder&&) = default;
  ~IgpackDecoder() = default;

  std::variant<DracoDecoder, IgpackExtractError> extract_draco_decoder(
      std::string asset_name) const;

  std::variant<const IgAsset::WgslSource*, IgpackExtractError>
  extract_wgsl_shader(const std::string& asset_name) const;

  std::variant<ozz::animation::Skeleton, IgpackExtractError>
  extract_ozz_skeleton(const std::string& asset_name) const;

  std::variant<OzzAnimationWithNames, IgpackExtractError> extract_ozz_animation(
      const std::string& asset_name) const;

  std::variant<RawHdr, IgpackExtractError> extract_raw_hdr(
      const std::string& asset_name) const;

 private:
  IgpackDecoder(std::string raw_data)
      : raw_data_(std::move(raw_data)),
        asset_pack_(IgAsset::GetAssetPack(&raw_data_[0])) {}

  std::string raw_data_;
  const IgAsset::AssetPack* asset_pack_;
};
}  // namespace igasset

#endif
