#ifndef IGDEMO_LOGIC_LEVELMETADATA_H
#define IGDEMO_LOGIC_LEVELMETADATA_H

#include <cstdint>

namespace igdemo {

struct CtxLevelMetadata {
  const float mapXMin;
  const float mapXRange;
  const float mapZMin;
  const float mapZRange;

  std::uint32_t rngBaseSeed;
};

}  // namespace igdemo

#endif
