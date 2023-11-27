#ifndef IGDEMO_PLATFORM_KEYBOARD_MOUSE_INPUT_EMITTER_H
#define IGDEMO_PLATFORM_KEYBOARD_MOUSE_INPUT_EMITTER_H

#include <GLFW/glfw3.h>
#include <igdemo/platform/input-emitter.h>

namespace igdemo {

class KeyboardMouseInputEmitter : public InputEmitter {
 public:
  KeyboardMouseInputEmitter(GLFWwindow* window) : window_(window) {}

  InputState get_input_state() const override;

 private:
  GLFWwindow* window_;
};

}  // namespace igdemo

#endif
