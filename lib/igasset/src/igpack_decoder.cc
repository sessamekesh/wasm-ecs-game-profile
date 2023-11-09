#include <igasset/igpack_decoder.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

namespace {

constexpr flatbuffers::Verifier::Options get_opts() {
  flatbuffers::Verifier::Options opts{};
  opts.assert = false;
  opts.check_alignment = false;
  opts.check_nested_flatbuffers = true;
  opts.max_depth = 8;
  opts.max_size = 512000000ull;

  return opts;
}
}  // namespace

namespace igasset {

std::shared_ptr<IgpackDecoder> IgpackDecoder::Create(std::string data) {
  const std::uint8_t* data_ptr =
      reinterpret_cast<const std::uint8_t*>(&data[0]);
  auto verifier = flatbuffers::Verifier(data_ptr, data.size(), ::get_opts());
  if (!IgAsset::VerifyAssetPackBuffer(verifier)) {
    return nullptr;
  }
  return std::shared_ptr<IgpackDecoder>(new IgpackDecoder(std::move(data)));
}

std::variant<DracoDecoder, IgpackExtractError>
IgpackDecoder::extract_draco_decoder(std::string asset_name) const {
  for (int i = 0; i < asset_pack_->assets()->size(); i++) {
    const auto* asset = asset_pack_->assets()->Get(i);
    if (asset->name()->str() != asset_name) {
      continue;
    }

    if (asset->data_type() != IgAsset::SingleAssetData_DracoGeometry) {
      return IgpackExtractError::WrongResourceType;
    }

    const auto* draco_asset = asset->data_as_DracoGeometry();

    std::vector<std::string> bone_names;
    std::vector<glm::mat4> bone_inv_bind_poses;

    for (int i = 0; i < draco_asset->ozz_bone_names()->size(); i++) {
      bone_names.push_back(draco_asset->ozz_bone_names()->Get(i)->str());
    }

    for (int i = 0; i < draco_asset->ozz_inv_bind_poses()->size(); i++) {
      glm::mat4 inv_bind_pose(1.f);
      for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
          inv_bind_pose[r][c] =
              draco_asset->ozz_inv_bind_poses()->Get(i)->values()->Get(r * 4 +
                                                                       c);
        }
      }

      bone_inv_bind_poses.push_back(inv_bind_pose);
    }

    auto decode_rsl = DracoDecoder::Create(
        reinterpret_cast<const char*>(draco_asset->draco_bin()->Data()),
        draco_asset->draco_bin()->size(), draco_asset->pos_attrib(),
        draco_asset->normal_attrib(),
        draco_asset->index_format() == IgAsset::IndexFormat_Uint16
            ? IndexBufferType::Uint16
            : IndexBufferType::Uint32,
        std::move(bone_names), std::move(bone_inv_bind_poses),
        draco_asset->tangent_attrib(), draco_asset->bitangent_attrib(),
        draco_asset->texcoord_attrib(), draco_asset->bone_idx_attrib(),
        draco_asset->bone_weight_attrib());
    if (std::holds_alternative<igasset::DracoDecoderError>(decode_rsl)) {
      return IgpackExtractError::AssetExtractError;
    }

    return std::get<DracoDecoder>(std::move(decode_rsl));
  }

  return IgpackExtractError::ResourceNotFound;
}

std::variant<const IgAsset::WgslSource*, IgpackExtractError>
IgpackDecoder::extract_wgsl_shader(const std::string& asset_name) const {
  for (int i = 0; i < asset_pack_->assets()->size(); i++) {
    const auto* asset = asset_pack_->assets()->Get(i);
    if (asset->name()->str() != asset_name) {
      continue;
    }

    if (asset->data_type() != IgAsset::SingleAssetData_WgslSource) {
      return IgpackExtractError::WrongResourceType;
    }

    return asset->data_as_WgslSource();
  }

  return IgpackExtractError::ResourceNotFound;
}

std::variant<ozz::animation::Skeleton, IgpackExtractError>
IgpackDecoder::extract_ozz_skeleton(const std::string& asset_name) const {
  for (int i = 0; i < asset_pack_->assets()->size(); i++) {
    const auto* asset = asset_pack_->assets()->Get(i);
    if (asset->name()->str() != asset_name) {
      continue;
    }

    if (asset->data_type() != IgAsset::SingleAssetData_OzzSkeleton) {
      return IgpackExtractError::WrongResourceType;
    }

    const auto* fb_ozz_skeleton = asset->data_as_OzzSkeleton();

    ozz::io::MemoryStream stream;
    stream.Write(fb_ozz_skeleton->ozz_data()->data(),
                 fb_ozz_skeleton->ozz_data()->size());
    stream.Seek(0, ozz::io::Stream::kSet);

    ozz::io::IArchive archive(&stream);
    if (!archive.TestTag<ozz::animation::Skeleton>()) {
      return IgpackExtractError::AssetExtractError;
    }

    ozz::animation::Skeleton skeleton;
    archive >> skeleton;

    return std::move(skeleton);
  }

  return IgpackExtractError::ResourceNotFound;
}

std::variant<OzzAnimationWithNames, IgpackExtractError>
IgpackDecoder::extract_ozz_animation(const std::string& asset_name) const {
  for (int i = 0; i < asset_pack_->assets()->size(); i++) {
    const auto* asset = asset_pack_->assets()->Get(i);
    if (asset->name()->str() != asset_name) {
      continue;
    }

    if (asset->data_type() != IgAsset::SingleAssetData_OzzAnimation) {
      return IgpackExtractError::WrongResourceType;
    }

    const auto* fb_ozz_animation = asset->data_as_OzzAnimation();

    ozz::io::MemoryStream stream;
    stream.Write(fb_ozz_animation->ozz_data()->data(),
                 fb_ozz_animation->ozz_data()->size());
    stream.Seek(0, ozz::io::Stream::kSet);

    ozz::io::IArchive archive(&stream);
    if (!archive.TestTag<ozz::animation::Animation>()) {
      return IgpackExtractError::AssetExtractError;
    }

    ozz::animation::Animation animation;
    archive >> animation;

    if (animation.num_tracks() != fb_ozz_animation->bone_names()->size()) {
      return IgpackExtractError::MalformedResourceData;
    }

    std::vector<std::string> animation_bone_names;
    animation_bone_names.reserve(fb_ozz_animation->bone_names()->size());
    for (int i = 0; i < fb_ozz_animation->bone_names()->size(); i++) {
      animation_bone_names.push_back(
          fb_ozz_animation->bone_names()->Get(i)->str());
    }

    return OzzAnimationWithNames{std::move(animation),
                                 std::move(animation_bone_names)};
  }

  return IgpackExtractError::ResourceNotFound;
}

}  // namespace igasset