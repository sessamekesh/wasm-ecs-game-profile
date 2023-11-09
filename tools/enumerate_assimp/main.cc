#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <format>
#include <iostream>

void print_scene_graph(const aiNode* node, int level, int total_levels) {
  if (level >= total_levels) return;
  for (int i = 0; i < level; i++) {
    std::cout << "--";
  }

  aiVector3D pos, scl;
  aiQuaternion rot;
  node->mTransformation.Decompose(scl, rot, pos);

  std::string spos = std::vformat("[{:.3f}, {:.3f}, {:.3f}]",
                                  std::make_format_args(pos.x, pos.y, pos.z));
  std::string srot =
      std::vformat("[{:.3f}, {:.3f}, {:.3f}, {:.3f}]",
                   std::make_format_args(rot.x, rot.y, rot.z, rot.w));
  std::string sscl = std::vformat("[{:.3f}, {:.3f}, {:.3f}]",
                                  std::make_format_args(scl.x, scl.y, scl.z));

  std::cout << " " << node->mName.C_Str() << " (T" << spos << " R" << srot
            << " S" << sscl << ")" << std::endl;
  for (int i = 0; i < node->mNumChildren; i++) {
    ::print_scene_graph(node->mChildren[i], level + 1, total_levels);
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr
        << "Incorrect arguments!\nUsage: list-assimp-assets [assimp file name]"
        << std::endl;
    return -1;
  }

  Assimp::Importer importer;
  importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);

  const aiScene* scene = importer.ReadFile(
      argv[1], aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs);

  if (scene == nullptr) {
    std::cerr << "No Assimp file found at given path" << std::endl;
    return -1;
  }

  aiMemoryInfo memory_info{};
  importer.GetMemoryRequirements(memory_info);

  std::cout << "Scene loaded: " << scene->mName.C_Str() << " ("
            << memory_info.total << " bytes)" << std::endl;

  std::cout << "\n--- MESHES (" << scene->mNumMeshes << ") ---" << std::endl;
  for (int i = 0; i < scene->mNumMeshes; i++) {
    std::cout << "(" << i << ") " << scene->mMeshes[i]->mName.C_Str() << " ("
              << scene->mMeshes[i]->mNumFaces << " tris)" << std::endl;
  }

  std::cout << "\n--- ANIMATIONS (" << scene->mNumAnimations << ") ---"
            << std::endl;
  for (int i = 0; i < scene->mNumAnimations; i++) {
    std::cout << "(" << i << ") " << scene->mAnimations[i]->mName.C_Str()
              << " (" << scene->mAnimations[i]->mNumChannels << " channels)"
              << std::endl;
  }

  std::cout << "\n--- TEXTURES (" << scene->mNumTextures << ") ---"
            << std::endl;
  for (int i = 0; i < scene->mNumTextures; i++) {
    std::cout << "(" << i << ") " << scene->mTextures[i]->mFilename.C_Str()
              << " (" << scene->mTextures[i]->mWidth << "x"
              << scene->mTextures[i]->mHeight << " "
              << scene->mTextures[i]->achFormatHint << ")" << std::endl;
  }

  std::cout << "\n\nScene graph (top 40 levels)\n";
  ::print_scene_graph(scene->mRootNode, 0, 40);

  return 0;
}
