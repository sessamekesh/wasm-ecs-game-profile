#include <GLFW/glfw3.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/platform/input-emitter.h>
#include <igdemo/render/camera.h>
#include <igdemo/systems/fly-camera.h>

namespace igdemo {

const igecs::WorldView::Decl& FlyCameraSystem::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           .ctx_reads<CtxActiveCamera>()
                                           .ctx_reads<CtxInputEmitter>()
                                           .ctx_reads<CtxFrameTime>()
                                           .writes<CameraComponent>();
  return decl;
}

void FlyCameraSystem::run(igecs::WorldView* wv) {
  const auto& activeCameraEntity =
      wv->ctx<CtxActiveCamera>().activeCameraEntity;
  auto& camera = wv->write<CameraComponent>(activeCameraEntity);
  const auto& event_emitter = wv->ctx<CtxInputEmitter>().input_emitter;
  const auto& dt = wv->ctx<CtxFrameTime>().secondsSinceLastFrame;

  auto fwd = glm::vec3(glm::cos(camera.phi) * glm::sin(camera.theta),
                       glm::sin(camera.phi),
                       glm::cos(camera.phi) * glm::cos(camera.theta));
  auto up = glm::vec3(0.f, 1.f, 0.f);
  auto right = glm::cross(fwd, up);

  auto input_state = event_emitter->get_input_state();

  camera.position += fwd * input_state.fwd * 30.f * dt;
  camera.position += right * input_state.right * 30.f * dt;

  camera.theta -= input_state.viewX * glm::radians(60.f) * dt;
  camera.phi += input_state.viewY * glm::radians(60.f) * dt;

  while (camera.theta > glm::radians(360.f)) {
    camera.theta -= glm::radians(360.f);
  }
  camera.phi = glm::clamp(camera.phi, glm::radians(-85.f), glm::radians(85.f));
}

}  // namespace igdemo
