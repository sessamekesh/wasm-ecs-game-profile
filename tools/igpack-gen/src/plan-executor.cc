#include <igpack-gen/plan-executor.h>

#include <fstream>
#include <iostream>
using namespace igpackgen;

namespace {
bool read_file(std::filesystem::path input_root_path, std::string filename,
               std::string& o) {
  auto path = input_root_path / filename;

  if (!std::filesystem::exists(path)) {
    std::cerr << "File not found at " << path << std::endl;
    return false;
  }

  std::ifstream fin(path, std::fstream::binary);
  if (!fin) {
    std::cerr << "Failed to open file at " << path << std::endl;
    return false;
  }
  fin.seekg(0, std::ios::end);
  size_t file_size = fin.tellg();
  fin.seekg(0, std::ios::beg);

  o.resize(file_size);
  if (!fin.read(reinterpret_cast<char*>(&o[0]), file_size)) {
    std::cerr << "Failed to read file at " << path << std::endl;
    return false;
  }

  return true;
}
}  // namespace

bool PlanExecutor::execute_plan(const PlanInvocationDesc& plan) {
  if (plan.Plan->actions() == nullptr) {
    std::clog << "Plan is empty - this may not be intended?" << std::endl;
    return true;
  }

  flatbuffers::FlatBufferBuilder fbb;
  std::vector<flatbuffers::Offset<IgAsset::SingleAsset>> assets;
  for (int i = 0; i < plan.Plan->actions()->size(); i++) {
    const auto& action = (*plan.Plan->actions())[i];

    switch (action->action_type()) {
      case IgpackGen::SingleActionData_CopyWgslSourceAction:
        if (!process_wgsl(plan.InputAssetPathRoot, fbb, assets,
                          action->action_as_CopyWgslSourceAction(),
                          action->igasset_name()->str())) {
          std::cerr << "Failed to process " << action->igasset_name()->str()
                    << std::endl;
          return false;
        }
        break;
      case IgpackGen::SingleActionData_AssimpToDracoAction:
        if (!process_assimp_to_draco(plan.InputAssetPathRoot, fbb, assets,
                                     action->action_as_AssimpToDracoAction(),
                                     action->igasset_name()->str())) {
          std::cerr << "Failed to process AssimpToDracoAction for "
                    << action->igasset_name()->str() << std::endl;
          return false;
        }
        break;
      case IgpackGen::SingleActionData_AssimpExtractOzzSkeleton:
        if (!process_assimp_to_ozz_skeleton(
                plan.InputAssetPathRoot, fbb, assets,
                action->action_as_AssimpExtractOzzSkeleton(),
                action->igasset_name()->str())) {
          std::cerr << "Failed to process AssimpExtractOzzSkeleton for "
                    << action->igasset_name()->str() << std::endl;
          return false;
        }
        break;
      case IgpackGen::SingleActionData_AssimpExtractOzzAnimation:
        if (!process_assimp_to_ozz_animation(
                plan.InputAssetPathRoot, fbb, assets,
                action->action_as_AssimpExtractOzzAnimation(),
                action->igasset_name()->str())) {
          std::cerr << "Failed to process AssimpExtractOzzAnimation for "
                    << action->igasset_name()->str() << std::endl;
          return false;
        }
        break;
      default:
        std::clog << "Unsupported action type found ("
                  << IgpackGen::EnumNameSingleActionData(action->action_type())
                  << ") - aborting" << std::endl;
        return false;
    }
  }

  // Done processing all elements of the plan! Pack and export.

  auto fb_assets = fbb.CreateVector(assets);

  auto asset_pack = IgAsset::CreateAssetPack(fbb, fb_assets);
  IgAsset::FinishAssetPackBuffer(fbb, asset_pack);

  std::filesystem::path out_path =
      plan.OutputAssetPathRoot / plan.Plan->output_path()->str();
  std::filesystem::path out_dir = out_path;
  out_dir.remove_filename();
  if (!std::filesystem::exists(out_dir)) {
    std::filesystem::create_directories(out_dir);
  }

  std::ofstream fout(out_path, std::fstream::binary);
  if (!fout.write(reinterpret_cast<const char*>(fbb.GetBufferPointer()),
                  fbb.GetSize())) {
    std::cerr << "Failed to write output igasset bundle to " << out_path
              << std::endl;
    return false;
  }

  return true;
}
bool PlanExecutor::process_wgsl(
    const std::filesystem::path& input_path_root,
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
    const IgpackGen::CopyWgslSourceAction* wgsl_action,
    std::string igasset_name) {
  std::string file_data;
  if (!read_file(input_path_root, wgsl_action->input_file_path()->str(),
                 file_data)) {
    return false;
  }
  return wgsl_.copy_wgsl_source(fbb, assets, *wgsl_action, igasset_name,
                                file_data);
}

bool PlanExecutor::process_assimp_to_draco(
    const std::filesystem::path& input_path_root,
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
    const IgpackGen::AssimpToDracoAction* action, std::string igasset_name) {
  std::string file_data;
  if (!read_file(input_path_root, action->input_file_path()->str(),
                 file_data)) {
    return false;
  }
  return assimp_geo_.export_draco_geo(fbb, assets, *action, igasset_name,
                                      file_data);
}

bool PlanExecutor::process_assimp_to_ozz_skeleton(
    const std::filesystem::path& input_path_root,
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
    const IgpackGen::AssimpExtractOzzSkeleton* action,
    std::string igasset_name) {
  std::string file_data;
  if (!read_file(input_path_root, action->input_file_path()->str(),
                 file_data)) {
    return false;
  }
  return assimp_animation_.export_skeleton(fbb, assets, *action, igasset_name,
                                           file_data);
}

bool PlanExecutor::process_assimp_to_ozz_animation(
    const std::filesystem::path& input_path_root,
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& assets,
    const IgpackGen::AssimpExtractOzzAnimation* action,
    std::string igasset_name) {
  std::string file_data;
  if (!read_file(input_path_root, action->input_file_path()->str(),
                 file_data)) {
    return false;
  }
  return assimp_animation_.export_animation(fbb, assets, *action, igasset_name,
                                            file_data);
}
