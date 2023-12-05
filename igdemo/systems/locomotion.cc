#include <igdemo/logic/locomotion.h>
#include <igdemo/logic/renderable.h>
#include <igdemo/render/world-transform-component.h>
#include <igdemo/systems/locomotion.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace igdemo {

const igecs::WorldView::Decl& LocomotionSystem::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           .reads<PositionComponent>()
                                           .reads<OrientationComponent>()
                                           .reads<ScaleComponent>()
                                           .writes<WorldTransformComponent>();

  return decl;
}

void LocomotionSystem::run(igecs::WorldView* wv) {
  auto view = wv->view<const PositionComponent, const OrientationComponent,
                       const ScaleComponent, WorldTransformComponent>();

  for (auto [e, p, o, r, wt] : view.each()) {
    glm::mat4 matScl = glm::scale(glm::vec3(r.scale));
    glm::mat4 matRot = glm::rotate(o.radAngle, glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 matPos =
        glm::translate(glm::vec3(p.map_position.x, 0.f, p.map_position.y));

    wt.worldTransform = matPos * matRot * matScl;
  }
}

}  // namespace igdemo
