#include <igasset/raw_hdr.h>

namespace igasset {

RawHdr::RawHdr(float* data, std::uint32_t width, std::uint32_t height,
               std::uint32_t num_components)
    : raw_data_(width * height * 4 * sizeof(numeric::float16_t), '\0'),
      width_(width),
      height_(height) {
  for (int i = 0; i < width * height * 4; i += 4) {
    for (int channel = 0; channel < 4; channel++) {
      size_t raw_idx = i + channel;

      numeric::float16_t* half = reinterpret_cast<numeric::float16_t*>(
          &raw_data_[raw_idx * sizeof(numeric::float16_t)]);

      size_t in_idx = i / 4 * num_components + channel;
      if (channel < num_components) {
        float* full = data + in_idx;
        *half = *full;
      } else {
        *half = 0.f;
      }
    }
  }
}

const numeric::float16_t* RawHdr::get_data() const {
  return reinterpret_cast<const numeric::float16_t*>(&raw_data_[0]);
}

std::size_t RawHdr::size() const { return raw_data_.size(); }

std::uint32_t RawHdr::width() const { return width_; }

std::uint32_t RawHdr::height() const { return height_; }

}  // namespace igasset
