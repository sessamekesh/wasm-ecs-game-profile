#include <igasset/draco_dec.h>

#include <glm/gtx/quaternion.hpp>

std::string igasset::to_string(DracoDecoderErrorType err_type) {
  switch (err_type) {
    case DracoDecoderErrorType::DecodeFailed:
      return "DecodeFailed";
    case DracoDecoderErrorType::MeshDataMissing:
      return "MeshDataMissing";
    case DracoDecoderErrorType::MeshDataInvalid:
      return "MeshDataInvalid";
    case DracoDecoderErrorType::NoData:
      return "NoData";
    case DracoDecoderErrorType::DrcPointCloudIsNotMesh:
      return "DrcPointCloudIsNotMesh";
    default:
      return "DracoDecoderErrorType-Unknown";
  }

  return "DracoDecoderErrorType-Unknown";
}

namespace igasset {

DracoDecoder::~DracoDecoder() = default;

std::variant<DracoDecoder, DracoDecoderError> DracoDecoder::Create(
    const char* draco_buffer, size_t draco_buffer_len, int position_attrib,
    int normal_quat_attrib, IndexBufferType index_buffer_type,
    std::vector<std::string> bone_names,
    std::vector<glm::mat4> bone_inv_bind_poses, int tangent_attrib,
    int bitangent_attrib, int texcoord_attrib, int bone_idx_attrib,
    int bone_weight_attrib) {
  draco::Decoder decoder;
  draco::DecoderBuffer drc_buffer;

  drc_buffer.Init(draco_buffer, draco_buffer_len);

  const auto type = draco::Decoder::GetEncodedGeometryType(&drc_buffer);
  if (!type.ok()) {
    return DracoDecoderError{DracoDecoderErrorType::DecodeFailed,
                             "Failed to infer Draco buffer type"};
  }

  if (type.value() != draco::TRIANGULAR_MESH) {
    return DracoDecoderError{
        DracoDecoderErrorType::DrcPointCloudIsNotMesh,
        "Draco buffer contains a point cloud, but no mesh"};
  }

  auto mesh_rsl = decoder.DecodeMeshFromBuffer(&drc_buffer);
  if (!mesh_rsl.ok()) {
    return DracoDecoderError{DracoDecoderErrorType::DecodeFailed,
                             mesh_rsl.status().error_msg()};
  }

  return DracoDecoder(std::move(mesh_rsl).value(), position_attrib,
                      normal_quat_attrib, tangent_attrib, bitangent_attrib,
                      texcoord_attrib, bone_idx_attrib, bone_weight_attrib,
                      index_buffer_type, std::move(bone_names),
                      std::move(bone_inv_bind_poses));
}

std::variant<std::vector<igasset::PosNormalVertexData3D>, DracoDecoderError>
DracoDecoder::get_pos_norm_data() const {
  if (position_attrib_ < 0 || normal_attrib_ < 0) {
    return DracoDecoderError{
        DracoDecoderErrorType::MeshDataMissing,
        "Invalid position_attrib or normal_attrib index value"};
  }

  auto pos_attrib = mesh_->GetNamedAttributeByUniqueId(
      draco::GeometryAttribute::POSITION, position_attrib_);
  auto normal_attrib = mesh_->GetNamedAttributeByUniqueId(
      draco::GeometryAttribute::NORMAL, normal_attrib_);

  if (!pos_attrib || !normal_attrib) {
    return DracoDecoderError{DracoDecoderErrorType::MeshDataMissing,
                             "Attribute at position or normal is missing"};
  }

  if (pos_attrib->num_components() != 3 ||
      pos_attrib->data_type() != draco::DataType::DT_FLOAT32) {
    return DracoDecoderError{DracoDecoderErrorType::MeshDataInvalid,
                             "Invalid position buffer format"};
  }

  if (normal_attrib->num_components() != 3 ||
      normal_attrib->data_type() != draco::DataType::DT_FLOAT32) {
    return DracoDecoderError{DracoDecoderErrorType::MeshDataInvalid,
                             "Invalid normal buffer format"};
  }

  std::vector<PosNormalVertexData3D> vertices;
  vertices.reserve(mesh_->num_points());

  for (draco::PointIndex vert_idx(0); vert_idx < mesh_->num_points();
       vert_idx++) {
    PosNormalVertexData3D vert{};

    pos_attrib->ConvertValue(pos_attrib->mapped_index(vert_idx), 3,
                             &vert.Position[0]);
    normal_attrib->ConvertValue(normal_attrib->mapped_index(vert_idx), 3,
                                &vert.Normal[0]);

    vertices.push_back(vert);
  }

  return vertices;
}

std::variant<std::vector<igasset::BoneWeightsVertexData>, DracoDecoderError>
DracoDecoder::get_bone_data() const {
  if (bone_idx_attrib_ < 0 || bone_weight_attrib_ < 0) {
    return DracoDecoderError{
        DracoDecoderErrorType::MeshDataMissing,
        "Invalid bone_idx_attrib or bone_weight_attrib index value"};
  }

  auto bone_idx_attrib = mesh_->GetNamedAttributeByUniqueId(
      draco::GeometryAttribute::GENERIC, bone_idx_attrib_);
  auto bone_weight_attrib = mesh_->GetNamedAttributeByUniqueId(
      draco::GeometryAttribute::GENERIC, bone_weight_attrib_);

  if (!bone_idx_attrib || !bone_weight_attrib) {
    return DracoDecoderError{DracoDecoderErrorType::MeshDataMissing,
                             "Attribute at bone_weight or bone_idx is missing"};
  }

  if (bone_idx_attrib->num_components() != 4 ||
      bone_idx_attrib->data_type() != draco::DataType::DT_UINT8) {
    return DracoDecoderError{DracoDecoderErrorType::MeshDataInvalid,
                             "Invalid bone_idx buffer format"};
  }

  if (bone_weight_attrib->num_components() != 4 ||
      bone_weight_attrib->data_type() != draco::DataType::DT_FLOAT32) {
    return DracoDecoderError{DracoDecoderErrorType::MeshDataInvalid,
                             "Invalid bone_weight buffer format"};
  }

  std::vector<BoneWeightsVertexData> vertices;
  vertices.reserve(mesh_->num_points());

  for (draco::PointIndex vert_idx(0); vert_idx < mesh_->num_points();
       vert_idx++) {
    BoneWeightsVertexData vert{};

    bone_weight_attrib->ConvertValue(bone_weight_attrib->mapped_index(vert_idx),
                                     4, &vert.Weights[0]);
    bone_idx_attrib->ConvertValue(bone_idx_attrib->mapped_index(vert_idx), 4,
                                  &vert.Indices[0]);

    vertices.push_back(vert);
  }

  return vertices;
}

std::variant<std::vector<std::string>, DracoDecoderError>
DracoDecoder::get_bone_names() const {
  if (bone_names_.size() == 0) {
    return DracoDecoderError{DracoDecoderErrorType::MeshDataMissing,
                             "Bone names not present on this model"};
  }

  return bone_names_;
}

std::variant<std::vector<glm::mat4>, DracoDecoderError>
DracoDecoder::get_bone_inv_bind_poses() const {
  if (bone_inv_bind_poses_.size() == 0) {
    return DracoDecoderError{DracoDecoderErrorType::MeshDataMissing,
                             "Bone names not present on this model"};
  }

  return bone_inv_bind_poses_;
}

std::variant<std::vector<std::uint16_t>, DracoDecoderError>
DracoDecoder::get_u16_indices() const {
  if (index_buffer_type_ != IndexBufferType::Uint16) {
    return DracoDecoderError{
        DracoDecoderErrorType::MeshDataMissing,
        "Model does not provide 16-bit indices (try 32-bit)"};
  }

  int num_faces = mesh_->num_faces();
  std::vector<std::uint16_t> indices;
  indices.reserve(num_faces * 3);
  for (draco::FaceIndex idx(0); idx < num_faces; idx++) {
    const auto& face = mesh_->face(idx);

    if (face.size() != 3) {
      return DracoDecoderError{DracoDecoderErrorType::MeshDataInvalid,
                               "Model has an invalid non-triangular face"};
    }

    for (const auto& face_index : face) {
      uint32_t index_value = face_index.value();
      if (index_value > 0xFFFFu) {
        return DracoDecoderError{
            DracoDecoderErrorType::MeshDataInvalid,
            "Uint16 overflow in buffer index - cannot unpack faces"};
      }

      indices.push_back(index_value);
    }
  }

  return indices;
}

std::variant<std::vector<std::uint32_t>, DracoDecoderError>
DracoDecoder::get_u32_indices() const {
  if (index_buffer_type_ != IndexBufferType::Uint32) {
    return DracoDecoderError{
        DracoDecoderErrorType::MeshDataMissing,
        "Model does not provide 32-bit indices (try 16-bit)"};
  }

  int num_faces = mesh_->num_faces();
  std::vector<std::uint32_t> indices;
  indices.reserve(num_faces * 3);
  for (draco::FaceIndex idx(0); idx < num_faces; idx++) {
    const auto& face = mesh_->face(idx);

    if (face.size() != 3) {
      return DracoDecoderError{DracoDecoderErrorType::MeshDataInvalid,
                               "Model has an invalid non-triangular face"};
    }

    for (const auto& face_index : face) {
      uint32_t index_value = face_index.value();
      indices.push_back(index_value);
    }
  }

  return indices;
}

DracoDecoder::DracoDecoder(std::unique_ptr<draco::Mesh> mesh,
                           int position_attrib, int normal_attrib,
                           int tangent_attrib, int bitangent_attrib,
                           int texcoord_attrib, int bone_idx_attrib,
                           int bone_weight_attrib,
                           IndexBufferType index_buffer_type,
                           std::vector<std::string> bone_names,
                           std::vector<glm::mat4> bone_inv_bind_poses)
    : mesh_(std::move(mesh)),
      position_attrib_(position_attrib),
      normal_attrib_(normal_attrib),
      tangent_attrib_(tangent_attrib),
      bitangent_attrib_(bitangent_attrib),
      texcoord_attrib_(texcoord_attrib),
      bone_idx_attrib_(bone_idx_attrib),
      bone_weight_attrib_(bone_weight_attrib),
      index_buffer_type_(index_buffer_type),
      bone_names_(std::move(bone_names)),
      bone_inv_bind_poses_(std::move(bone_inv_bind_poses)) {}

}  // namespace igasset