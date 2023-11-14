#include <igasset/raw_hdr.h>

namespace igasset {

RawHdr::RawHdr(float* data, std::uint32_t width, std::uint32_t height,
               std::uint32_t num_components)
    : raw_data_(width * height * num_components * sizeof(numeric::float16_t),
                '\0'),
      width_(width),
      height_(height),
      num_channels_(num_components) {
  for (int i = 0; i < width * height * num_components; i++) {
    numeric::float16_t* half = reinterpret_cast<numeric::float16_t*>(
        &raw_data_[i * sizeof(numeric::float16_t)]);
    float* full = data + i;
    *half = *full;
  }
  memcpy_s(&raw_data_[0], raw_data_.size(), data,
           width * height * num_components * 4);
}

const numeric::float16_t* RawHdr::get_data() const {
  return reinterpret_cast<const numeric::float16_t*>(&raw_data_[0]);
}

std::size_t RawHdr::size() const { return raw_data_.size(); }

std::uint32_t RawHdr::width() const { return width_; }

std::uint32_t RawHdr::height() const { return height_; }

std::uint32_t RawHdr::num_channels() const { return num_channels_; }

}  // namespace igasset
