#ifndef IGASSET_RAW_HDR_H
#define IGASSET_RAW_HDR_H

#include <float16_t.hpp>
#include <glm/glm.hpp>
#include <string>

namespace igasset {

class RawHdr {
 public:
  RawHdr(float* data, std::uint32_t width, std::uint32_t height,
         std::uint32_t num_components);

  const numeric::float16_t* get_data() const;
  std::size_t size() const;
  std::uint32_t width() const;
  std::uint32_t height() const;
  std::uint32_t num_channels() const;

  RawHdr() = delete;
  ~RawHdr() = default;
  RawHdr(const RawHdr&) = delete;
  RawHdr& operator=(const RawHdr&) = delete;
  RawHdr(RawHdr&&) = default;
  RawHdr& operator=(RawHdr&&) = default;

 private:
  // Why std::string and not std::vector<float>?
  // https://github.com/sessamekesh/slow-wasm-vec-dtor
  std::string raw_data_;
  std::uint32_t width_;
  std::uint32_t height_;
  std::uint32_t num_channels_;
};

}  // namespace igasset

#endif
