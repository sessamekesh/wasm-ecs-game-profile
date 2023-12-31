#include <igdemo/assets/projectiles.h>
#include <igdemo/assets/ybot.h>
#include <igdemo/logic/projectile.h>
#include <igdemo/logic/renderable.h>
#include <igdemo/render/animated-pbr.h>
#include <igdemo/systems/attach-renderables.h>

namespace igdemo {

const igecs::WorldView::Decl& AttachRenderablesSystem::decl() {
  static igecs::WorldView::Decl decl =
      igecs::WorldView::Decl()
          .reads<RenderableComponent>()
          .reads<Projectile>()
          .merge_in_decl(YbotRenderResources::decl())
          .merge_in_decl(YbotAnimationResources::decl())
          .merge_in_decl(ProjectileRenderUtil::decl());

  return decl;
}

void AttachRenderablesSystem::run(igecs::WorldView* wv) {
  {
    auto view = wv->view<const RenderableComponent>();

    for (auto [e, rc] : view.each()) {
      // Make sure geometry is attached...
      switch (rc.type) {
        case ModelType::YBOT:
        default:
          if (!YbotRenderResources::has_render_resources(wv, e)) {
            YbotRenderResources::attach(wv, e, rc.material);
          }
          break;
      }

      // Make sure animation is correct...
      switch (rc.type) {
        case ModelType::YBOT:
        default:
          YbotAnimationResources::update_animation_state(wv, e, rc.animation);
          break;
      }
    }
  }

  {
    auto view = wv->view<const Projectile>();
    for (auto [e, rc] : view.each()) {
      if (!ProjectileRenderUtil::has_render_resources(wv, e)) {
        ProjectileRenderUtil::attach_render_resources(wv, e);
      }
    }
  }
}

}  // namespace igdemo
