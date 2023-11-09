#ifndef IGASSET_IGPACK_DECODER_H
#define IGASSET_IGPACK_DECODER_H

#include <igasset/draco_dec.h>
#include <igasset/ozz_wrappers.h>
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

  // TODO (sessamekesh): extract OZZ skeletons and animations
  // NOTICE: Bone names in skeletons are not necessarily in line with
  //  animation channels! This is a feature of igpack-loader, and
  //  possibly a stupid one that should be re-thought to push work
  //  to the asset pipeline packing stage instead of runtime loading.
  // It may or may not be feasible to unpack skeletons/animations here.
  // It's totally feasible, do something like this:
  // https://github.com/guillaumeblanc/ozz-animation/blob/feature/magic/samples/additive/sample_additive.cc#L163-L175
  // Basically...
  //  - Sample the animation as normal with ozz::animation::SamplingJob
  //  - Remap from animation to skeleton index space
  //  - Obtain skeleton LTM transforms with ozz::animation::LocalToModelJob
  //  - Remap from skeleton to model index space
  //  - Upload to GPU
  // Remapping should be _fairly_ efficient, but it is unnecessary work
  //  if everything was all in the same index space to begin with.
  // As long as there isn't crazy animation CPU load (profile!) this isn't
  //  worth updating for this specific game (marketable-plushies-game).

 private:
  IgpackDecoder(std::string raw_data)
      : raw_data_(std::move(raw_data)),
        asset_pack_(IgAsset::GetAssetPack(&raw_data_[0])) {}

  std::string raw_data_;
  const IgAsset::AssetPack* asset_pack_;
};
}  // namespace igasset

#endif
