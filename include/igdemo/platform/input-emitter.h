#ifndef IGDEMO_PLATFORM_INPUT_EMITTER_H
#define IGDEMO_PLATFORM_INPUT_EMITTER_H

#include <glm/glm.hpp>
#include <memory>

namespace igdemo {

struct InputState {
  float fwd;
  float right;

  float viewX;
  float viewY;
};

class InputEmitter {
 public:
  virtual InputState get_input_state() const = 0;
};

struct CtxInputEmitter {
  std::unique_ptr<InputEmitter> input_emitter;
};

}  // namespace igdemo

#endif
