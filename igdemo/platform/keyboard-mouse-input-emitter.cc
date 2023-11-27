#include <igdemo/platform/keyboard-mouse-input-emitter.h>

namespace igdemo {

InputState KeyboardMouseInputEmitter::get_input_state() const {
  InputState input_state{};

  if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
    input_state.fwd += 1.f;
  }
  if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
    input_state.fwd -= 1.f;
  }

  if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
    input_state.right -= 1.f;
  }
  if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
    input_state.right += 1.f;
  }

  if (glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS) {
    input_state.viewY += 1.f;
  }
  if (glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS) {
    input_state.viewY -= 1.f;
  }

  if (glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS) {
    input_state.viewX -= 1.f;
  }
  if (glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    input_state.viewX += 1.f;
  }

  return input_state;
}

}  // namespace igdemo
