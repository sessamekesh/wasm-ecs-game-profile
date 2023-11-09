#include <assimp/postprocess.h>
#include <igpack-gen/assimp-util.h>

#include <iostream>

namespace igpackgen {

std::shared_ptr<AssimpSceneData> igpackgen::load_scene(
    const std::string& file_data, const std::string& igasset_name) {
  if (file_data.size() == 0u) {
    std::cerr << "Failed to process empty file into AssimpSceneData"
              << std::endl;
    return nullptr;
  }

  auto tr = std::make_shared<AssimpSceneData>();
  tr->importer = std::make_shared<Assimp::Importer>();
  tr->importer->SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
  // Import speed isn't super important for the build step compared to final
  // output - we can pay the extra cost to generate tangent/bitangents, remove
  // degenerate polygons, triangulate the mesh, etc.
  const uint32_t import_flags = aiProcessPreset_TargetRealtime_Quality |
                                aiProcess_FlipUVs | aiProcess_CalcTangentSpace |
                                aiProcess_FixInfacingNormals;

  tr->scene = tr->importer->ReadFileFromMemory(&file_data[0], file_data.size(),
                                               import_flags);

  if (!tr->scene) {
    std::cerr << "Failed to process geometry requested for " << igasset_name
              << ": " << tr->importer->GetErrorString() << std::endl;
    return nullptr;
  }

  return tr;
}

}  // namespace igpackgen