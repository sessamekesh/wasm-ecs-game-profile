#include <assimp/scene.h>
#include <draco/compression/encode.h>
#include <igasset/vertex_types.h>
#include <igpack-gen/assimp-geo-processor.h>
#include <igpack-gen/assimp-util.h>

#include <assimp/Importer.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

using namespace igpackgen;

namespace {

struct BoneMeta {
  std::string name;
  glm::mat4 invBindPose;
};

struct BaseGeoData {
  std::vector<igasset::PosNormalVertexData3D> posNormData;
  std::vector<igasset::TangentBitangentVertexData3D> tbData;
  std::vector<igasset::TexcoordVertexData> texcoordData;
  std::vector<igasset::BoneWeightsVertexData> boneWeightsData;
  std::vector<BoneMeta> boneMetadata;
  std::vector<uint32_t> indexData;
};

std::shared_ptr<BaseGeoData> extract_base_geo(const aiMesh* mesh) {
  std::vector<igasset::PosNormalVertexData3D> pos_norm_data;
  std::vector<igasset::TangentBitangentVertexData3D> tb_data;
  std::vector<igasset::TexcoordVertexData> texcoord_data;
  std::vector<igasset::BoneWeightsVertexData> bone_weights_data;
  std::vector<BoneMeta> bone_metadata;
  std::vector<uint32_t> index_data;

  pos_norm_data.reserve(mesh->mNumVertices);
  tb_data.reserve(mesh->mNumVertices);
  texcoord_data.reserve(mesh->mNumVertices);
  bone_weights_data.reserve(mesh->mNumVertices);
  bone_metadata.reserve(mesh->mNumBones);
  index_data.reserve(mesh->mNumFaces * 3);

  for (std::uint32_t idx = 0; idx < mesh->mNumVertices; idx++) {
    igasset::PosNormalVertexData3D data{};
    data.Position.x = mesh->mVertices[idx].x;
    data.Position.y = mesh->mVertices[idx].y;
    data.Position.z = mesh->mVertices[idx].z;
    data.Normal.x = mesh->mNormals[idx].x;
    data.Normal.y = mesh->mNormals[idx].y;
    data.Normal.z = mesh->mNormals[idx].z;

    if (mesh->HasTangentsAndBitangents()) {
      igasset::TangentBitangentVertexData3D tb{};
      tb.Tangent.x = mesh->mTangents[idx].x;
      tb.Tangent.y = mesh->mTangents[idx].y;
      tb.Tangent.z = mesh->mTangents[idx].z;
      tb.Bitangent.x = mesh->mBitangents[idx].x;
      tb.Bitangent.y = mesh->mBitangents[idx].y;
      tb.Bitangent.z = mesh->mBitangents[idx].z;
      tb_data.push_back(tb);
    }

    pos_norm_data.push_back(data);

    if (mesh->HasTextureCoords(0)) {
      igasset::TexcoordVertexData tc_data{};
      tc_data.Texcoord.x = mesh->mTextureCoords[0][idx].x;
      tc_data.Texcoord.y = mesh->mTextureCoords[0][idx].y;
      texcoord_data.push_back(tc_data);
    }

    if (mesh->HasBones()) {
      igasset::BoneWeightsVertexData bw_data{};
      bw_data.Indices = {0u, 0u, 0u, 0u};
      bw_data.Weights = {0.f, 0.f, 0.f, 0.f};
      bone_weights_data.push_back(bw_data);
    }
  }

  for (std::uint32_t idx = 0; idx < mesh->mNumFaces; idx++) {
    if (mesh->mFaces[idx].mNumIndices != 3) {
      std::cerr << "Non-triangular face encountered in mesh "
                << mesh->mName.C_Str() << std::endl;
      return nullptr;
    }
    index_data.push_back(mesh->mFaces[idx].mIndices[0]);
    index_data.push_back(mesh->mFaces[idx].mIndices[1]);
    index_data.push_back(mesh->mFaces[idx].mIndices[2]);
  }

  if (mesh->HasBones()) {
    for (std::uint32_t bone_idx = 0; bone_idx < mesh->mNumBones; bone_idx++) {
      const auto& bone = mesh->mBones[bone_idx];

      std::uint32_t weights_to_process = bone->mNumWeights;
      for (std::uint32_t widx = 0; widx < weights_to_process; widx++) {
        const auto& weight = bone->mWeights[widx];
        auto& o_vertex = bone_weights_data[weight.mVertexId];

        if (o_vertex.Weights[0] <= 0.f) {
          o_vertex.Indices[0] = bone_idx;
          o_vertex.Weights[0] = weight.mWeight;
        } else if (o_vertex.Weights[1] <= 0.f) {
          o_vertex.Indices[1] = bone_idx;
          o_vertex.Weights[1] = weight.mWeight;
        } else if (o_vertex.Weights[2] <= 0.f) {
          o_vertex.Indices[2] = bone_idx;
          o_vertex.Weights[2] = weight.mWeight;
        } else if (o_vertex.Weights[3] <= 0.f) {
          o_vertex.Indices[3] = bone_idx;
          o_vertex.Weights[3] = weight.mWeight;
        } else {
          std::cerr << "[AssimpGeoProcessor] Error compiling bone associations "
                       "- vertex "
                    << weight.mVertexId << " has too many bone associations"
                    << std::endl;
          return nullptr;
        }
      }

      // Export bone data...
      const auto& inv = bone->mOffsetMatrix;
      glm::mat4 o_inv{};
      for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
          o_inv[r][c] = inv[c][r];
        }
      }
      bone_metadata.push_back(BoneMeta{bone->mName.C_Str(), o_inv});
    }
  }

  std::shared_ptr<BaseGeoData> out = std::make_shared<BaseGeoData>();
  out->posNormData = std::move(pos_norm_data);
  out->tbData = std::move(tb_data);
  out->texcoordData = std::move(texcoord_data);
  out->boneWeightsData = std::move(bone_weights_data);
  out->boneMetadata = std::move(bone_metadata);
  out->indexData = std::move(index_data);
  return out;
}

const aiMesh* load_mesh_from_scene(
    std::shared_ptr<AssimpSceneData> maybe_scene_data,
    const std::string& mesh_name, const std::string& igasset_name) {
  if (!maybe_scene_data) {
    return nullptr;
  }

  const aiScene* scene = maybe_scene_data->scene;

  for (int i = 0; i < scene->mNumMeshes; i++) {
    const aiMesh* mesh = scene->mMeshes[i];

    // Optimization: skip all non-TRIANGLES (Assimp often imports
    // LINE types first)
    if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
      continue;
    }

    if (mesh->mName.C_Str() == mesh_name) {
      return mesh;
    }
  }

  return nullptr;
}

}  // namespace

bool AssimpGeoProcessor::export_draco_geo(
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
    const IgpackGen::AssimpToDracoAction& action,
    const std::string& igasset_name, const std::string& assimp_file_raw) {
  std::string file_name = action.input_file_path()->str();

  std::shared_ptr<AssimpSceneData> scene =
      ::load_scene(assimp_file_raw, igasset_name);
  if (!scene) {
    std::cerr << "Could not load scene for " << igasset_name << std::endl;
    return false;
  }

  if (!action.assimp_mesh_names()) {
    std::cerr << "No mesh names provided - cannot convert static geo "
              << igasset_name << std::endl;
    return false;
  }

  bool has_texcoords = false;
  bool has_bones = false;
  uint32_t num_vertices = 0u;
  uint32_t num_faces = 0u;

  uint32_t index_offset = 0u;
  for (int i = 0; i < action.assimp_mesh_names()->size(); i++) {
    std::string mesh_name = action.assimp_mesh_names()->Get(i)->str();

    const aiMesh* mesh = ::load_mesh_from_scene(scene, mesh_name, igasset_name);
    if (!mesh) {
      std::cerr << "Mesh " << mesh_name << " not found in scene for "
                << igasset_name << std::endl;
      return false;
    }

    if (i == 0) {
      has_texcoords = mesh->HasTextureCoords(0);
      has_bones = mesh->HasBones();
    } else {
      if (has_texcoords != mesh->HasTextureCoords(0)) {
        std::cerr << "Texcoord presence mismatch - aborting mesh " << mesh_name
                  << std::endl;
        return false;
      }
      if (has_bones != mesh->HasBones()) {
        std::cerr << "Bone presence mismatch - aborting mesh " << mesh_name
                  << std::endl;
        return false;
      }
    }
    num_vertices += mesh->mNumVertices;
    num_faces += mesh->mNumFaces;
  }

  std::vector<igasset::PosNormalVertexData3D> pos_norm_data;
  std::vector<igasset::TangentBitangentVertexData3D> tb_data;
  std::vector<igasset::TexcoordVertexData> texcoord_data;
  std::vector<igasset::BoneWeightsVertexData> bone_data;
  std::vector<BoneMeta> bone_meta;
  std::vector<std::uint32_t> index_data;
  pos_norm_data.reserve(num_vertices);
  tb_data.reserve(num_vertices);
  texcoord_data.reserve(has_texcoords ? num_vertices : 1u);
  index_data.reserve(num_faces * 3u);

  for (int i = 0; i < action.assimp_mesh_names()->size(); i++) {
    std::string mesh_name = action.assimp_mesh_names()->Get(i)->str();

    const aiMesh* mesh = ::load_mesh_from_scene(scene, mesh_name, igasset_name);
    if (!mesh) {
      std::cerr << "Mesh " << mesh_name << " not found in scene for "
                << igasset_name << std::endl;
      return false;
    }

    auto geo_data = ::extract_base_geo(mesh);
    if (!geo_data) {
      std::cerr << "Failed to extract relevant static geo data for "
                << igasset_name << " (mesh " << mesh_name << ")" << std::endl;
      return false;
    }
    for (int j = 0; j < geo_data->posNormData.size(); j++) {
      pos_norm_data.push_back(geo_data->posNormData[j]);
    }
    for (int j = 0; j < geo_data->tbData.size(); j++) {
      tb_data.push_back(geo_data->tbData[j]);
    }
    for (int j = 0; j < geo_data->texcoordData.size(); j++) {
      texcoord_data.push_back(geo_data->texcoordData[j]);
    }
    for (int j = 0; j < geo_data->boneWeightsData.size(); j++) {
      bone_data.push_back(geo_data->boneWeightsData[j]);
    }
    for (int j = 0; j < geo_data->indexData.size(); j++) {
      index_data.push_back(geo_data->indexData[j] + index_offset);
    }
    index_offset += mesh->mNumVertices;

    if (i == 0) {
      bone_meta = geo_data->boneMetadata;
    } else {
      if (bone_meta.size() != geo_data->boneMetadata.size()) {
        std::cerr << "Bone metadata mismatch, cannot extract bone bindings for "
                  << mesh_name << std::endl;
        return false;
      }
      for (int j = 0; j < geo_data->boneMetadata.size(); j++) {
        if (bone_meta[j].name != geo_data->boneMetadata[j].name) {
          std::cerr << "Bone name mismatch in mesh " << mesh_name << " at bone "
                    << geo_data->boneMetadata[j].name << " (expected "
                    << bone_meta[j].name << ")" << std::endl;
          return false;
        }
      }
    }
  }

  if (index_data.size() % 3 != 0) {
    std::cerr << "Invalid number of triangle indices (" << index_data.size()
              << ") for " << igasset_name << std::endl;
    return false;
  }

  if (index_data.size() == 0) {
    std::cerr << "No faces found for " << igasset_name << std::endl;
    return false;
  }

  //
  // Draco encoding...
  //
  draco::Mesh draco_mesh;
  draco_mesh.set_num_points(pos_norm_data.size());

  draco::DataBuffer pos_norm_data_buffer;
  pos_norm_data_buffer.Update(
      &pos_norm_data[0],
      pos_norm_data.size() * sizeof(igasset::PosNormalVertexData3D));

  int pos_attribute_idx = -1;
  int normal_attribute_idx = -1;
  int tangent_attribute_idx = -1;
  int bitangent_attribute_idx = -1;
  int texcoord_attribute_idx = -1;
  int bone_idx_attrib_idx = -1;
  int bone_weight_attrib_idx = -1;

  // position attribute
  {
    draco::GeometryAttribute pos_attribute;
    pos_attribute.Init(draco::GeometryAttribute::POSITION,
                       &pos_norm_data_buffer, 3, draco::DT_FLOAT32, false,
                       sizeof(igasset::PosNormalVertexData3D),
                       offsetof(igasset::PosNormalVertexData3D, Position));
    pos_attribute_idx =
        draco_mesh.AddAttribute(pos_attribute, true, pos_norm_data.size());
    if (pos_attribute_idx < 0) {
      std::cerr << "Failed to set up Draco position attribute for "
                << igasset_name << std::endl;
      return false;
    }
  }

  // normal attribute
  {
    draco::GeometryAttribute normal_attribute;
    normal_attribute.Init(draco::GeometryAttribute::NORMAL,
                          &pos_norm_data_buffer, 3, draco::DT_FLOAT32, false,
                          sizeof(igasset::PosNormalVertexData3D),
                          offsetof(igasset::PosNormalVertexData3D, Normal));
    normal_attribute_idx =
        draco_mesh.AddAttribute(normal_attribute, true, pos_norm_data.size());
    if (normal_attribute_idx < 0) {
      std::cerr << "Failed to set up Draco normal quat attribute for "
                << igasset_name << std::endl;
      return false;
    }
  }

  // Set mappings...
  for (uint32_t i = 0u; i < pos_norm_data.size(); i++) {
    draco_mesh.attribute(pos_attribute_idx)
        ->SetAttributeValue(draco::AttributeValueIndex(i),
                            &pos_norm_data[i].Position);
    draco_mesh.attribute(normal_attribute_idx)
        ->SetAttributeValue(draco::AttributeValueIndex(i),
                            &pos_norm_data[i].Normal);
  }

  if (action.include_tangent_bitangents()) {
    draco::DataBuffer tb_data_buffer;
    tb_data_buffer.Update(
        &tb_data[0],
        tb_data.size() * sizeof(igasset::TangentBitangentVertexData3D));
    {
      draco::GeometryAttribute tangent_attribute;
      tangent_attribute.Init(
          draco::GeometryAttribute::GENERIC, &tb_data_buffer, 3,
          draco::DT_FLOAT32, false,
          sizeof(igasset::TangentBitangentVertexData3D),
          offsetof(igasset::TangentBitangentVertexData3D, Tangent));
      tangent_attribute_idx =
          draco_mesh.AddAttribute(tangent_attribute, true, tb_data.size());
      if (tangent_attribute_idx < 0) {
        std::cerr << "Failed to set up Draco normal quat attribute for "
                  << igasset_name << std::endl;
        return false;
      }
    }

    {
      draco::GeometryAttribute bitangent_attribute;
      bitangent_attribute.Init(
          draco::GeometryAttribute::GENERIC, &tb_data_buffer, 3,
          draco::DT_FLOAT32, false, sizeof(igasset::PosNormalVertexData3D),
          offsetof(igasset::TangentBitangentVertexData3D, Bitangent));
      bitangent_attribute_idx =
          draco_mesh.AddAttribute(bitangent_attribute, true, tb_data.size());
      if (bitangent_attribute_idx < 0) {
        std::cerr << "Failed to set up Draco normal quat attribute for "
                  << igasset_name << std::endl;
        return false;
      }
    }
  }

  // texcoord attribute
  if (has_texcoords && action.include_texcoords()) {
    draco::DataBuffer texcoord_databuffer;
    texcoord_databuffer.Update(
        &texcoord_data[0],
        texcoord_data.size() * sizeof(igasset::TexcoordVertexData));

    draco::GeometryAttribute uv_attribute;
    uv_attribute.Init(draco::GeometryAttribute::TEX_COORD, &texcoord_databuffer,
                      2, draco::DT_FLOAT32, false,
                      sizeof(igasset::TexcoordVertexData),
                      offsetof(igasset::TexcoordVertexData, Texcoord));

    texcoord_attribute_idx =
        draco_mesh.AddAttribute(uv_attribute, true, texcoord_data.size());
    for (int i = 0; i < texcoord_data.size(); i++) {
      draco_mesh.attribute(texcoord_attribute_idx)
          ->SetAttributeValue(draco::AttributeValueIndex(i),
                              &texcoord_data[i].Texcoord);
    }
  }

  if (has_bones && action.include_bones()) {
    draco::DataBuffer bone_data_buffer;
    bone_data_buffer.Update(
        &bone_data[0],
        bone_data.size() * sizeof(igasset::BoneWeightsVertexData));

    draco::GeometryAttribute bone_idx_attrib;
    bone_idx_attrib.Init(draco::GeometryAttribute::GENERIC, &bone_data_buffer,
                         4, draco::DT_UINT8, false,
                         sizeof(igasset::BoneWeightsVertexData),
                         offsetof(igasset::BoneWeightsVertexData, Indices));

    bone_idx_attrib_idx =
        draco_mesh.AddAttribute(bone_idx_attrib, true, bone_data.size());

    draco::GeometryAttribute bone_weight_attrib;
    bone_weight_attrib.Init(draco::GeometryAttribute::GENERIC,
                            &bone_data_buffer, 4, draco::DT_FLOAT32, false,
                            sizeof(igasset::BoneWeightsVertexData),
                            offsetof(igasset::BoneWeightsVertexData, Weights));

    bone_weight_attrib_idx =
        draco_mesh.AddAttribute(bone_weight_attrib, true, bone_data.size());

    for (int i = 0; i < bone_data.size(); i++) {
      draco_mesh.attribute(bone_idx_attrib_idx)
          ->SetAttributeValue(draco::AttributeValueIndex(i),
                              &bone_data[i].Indices);
      draco_mesh.attribute(bone_weight_attrib_idx)
          ->SetAttributeValue(draco::AttributeValueIndex(i),
                              &bone_data[i].Weights);
    }
  }

  // faces
  draco_mesh.SetNumFaces(index_data.size() / 3);
  for (int i = 0; i < index_data.size() / 3; i++) {
    draco::Mesh::Face face;
    face[0] = index_data[i * 3];
    face[1] = index_data[i * 3 + 1];
    face[2] = index_data[i * 3 + 2];
    draco_mesh.SetFace(draco::FaceIndex(i), face);
  }

#ifdef DRACO_ATTRIBUTE_VALUES_DEDUPLICATION_SUPPORTED
  draco_mesh.DeduplicateAttributeValues();
#endif
#ifdef DRACO_ATTRIBUTE_INDICES_DEDUPLICATION_SUPPORTED
  draco_mesh.DeduplicatePointIds();
#endif

  draco::Encoder encoder;

  int encoding_speed = std::min(
      10, std::max(0, 10 - action.draco_params()->compression_level()));
  int decoding_speed = std::min(
      10, std::max(0, 10 - action.draco_params()->decompression_level()));

  encoder.SetSpeedOptions(encoding_speed, decoding_speed);
  encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION,
                                   action.draco_params()->pos_quantization());
  encoder.SetAttributeQuantization(
      draco::GeometryAttribute::TEX_COORD,
      action.draco_params()->texcoord_quantization());
  encoder.SetAttributeQuantization(
      draco::GeometryAttribute::GENERIC,
      action.draco_params()->general_quantization());
  encoder.SetAttributeQuantization(
      draco::GeometryAttribute::NORMAL,
      action.draco_params()->normal_quantization());

  draco::EncoderBuffer out_buffer;
  auto status = encoder.EncodeMeshToBuffer(draco_mesh, &out_buffer);
  if (!status.ok()) {
    std::cerr << "Failure in Draco encoder for " << igasset_name << ": "
              << status.error_msg_string() << std::endl;
    return false;
  }

  auto index_type = (index_offset > 0xFFFF) ? IgAsset::IndexFormat_Uint32
                                            : IgAsset::IndexFormat_Uint16;

  auto fb_draco_bin =
      fbb.CreateVector(reinterpret_cast<const std::uint8_t*>(out_buffer.data()),
                       out_buffer.size());
  std::vector<flatbuffers::Offset<flatbuffers::String>> fb_bone_names;
  std::vector<flatbuffers::Offset<IgAsset::Mat4>> fb_inv_bind_poses;
  for (int i = 0; i < bone_meta.size(); i++) {
    fb_bone_names.push_back(fbb.CreateString(bone_meta[i].name));
    auto fb_inv_bind_pose_vec =
        fbb.CreateVector(&bone_meta[i].invBindPose[0][0], 16 * sizeof(float));
    auto fb_inv_bind_pose_mat4 = IgAsset::CreateMat4(fbb, fb_inv_bind_pose_vec);
    fb_inv_bind_poses.push_back(fb_inv_bind_pose_mat4);
  }

  auto fb_ozz_bone_names = fbb.CreateVector(fb_bone_names);
  auto fb_ozz_inv_bind_poses = fbb.CreateVector(fb_inv_bind_poses);

  auto fb_draco_geo = IgAsset::CreateDracoGeometry(
      fbb, pos_attribute_idx, normal_attribute_idx, tangent_attribute_idx,
      bitangent_attribute_idx, texcoord_attribute_idx, bone_idx_attrib_idx,
      bone_weight_attrib_idx, fb_draco_bin, index_type, fb_ozz_bone_names,
      fb_ozz_inv_bind_poses);
  auto fb_name = fbb.CreateString(igasset_name);

  auto fb_single_asset = IgAsset::CreateSingleAsset(
      fbb, fb_name, IgAsset::SingleAssetData_DracoGeometry,
      fb_draco_geo.Union());

  asset_list.push_back(fb_single_asset);

  return true;
}
