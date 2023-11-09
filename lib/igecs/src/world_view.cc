#include <igecs/ctti_type_id.h>
#include <igecs/world_view.h>

namespace igecs {

WorldView::Decl WorldView::Decl::Thin() { return Decl(true); }

WorldView::Decl::Decl() : allow_all_(false) {}

WorldView::Decl::Decl(bool allow_all) : allow_all_(allow_all) {}

WorldView WorldView::Decl::create(entt::registry* registry) const {
  return WorldView(registry, *this);
}

WorldView::WorldView(entt::registry* registry, Decl decl)
    : registry_(registry), decl_(std::move(decl)) {
  assert(registry != nullptr);
}

WorldView::Decl& WorldView::Decl::merge_in_decl(const WorldView::Decl& o) {
#ifdef IG_ENABLE_ECS_VALIDATION
  for (int i = 0; i < o.reads_.size(); i++) {
    if (!::vec_contains(reads_, o.reads_[i])) {
      reads_.push_back(o.reads_[i]);
    }
  }

  for (int i = 0; i < o.writes_.size(); i++) {
    if (!::vec_contains(writes_, o.writes_[i])) {
      writes_.push_back(o.writes_[i]);
    }
  }

  for (int i = 0; i < o.ctx_reads_.size(); i++) {
    if (!::vec_contains(ctx_reads_, o.ctx_reads_[i])) {
      ctx_reads_.push_back(o.ctx_reads_[i]);
    }
  }

  for (int i = 0; i < o.ctx_writes_.size(); i++) {
    if (!::vec_contains(ctx_writes_, o.ctx_writes_[i])) {
      ctx_writes_.push_back(o.ctx_writes_[i]);
    }
  }

  for (int i = 0; i < o.evt_writes_.size(); i++) {
    if (!::vec_contains(evt_writes_, o.evt_writes_[i])) {
      evt_writes_.push_back(o.evt_writes_[i]);
    }
  }

  for (int i = 0; i < o.evt_consumes_.size(); i++) {
    if (!::vec_contains(evt_consumes_, o.evt_consumes_[i])) {
      evt_consumes_.push_back(o.evt_consumes_[i]);
    }
  }
#endif
  return *this;
}

WorldView WorldView::Thin(entt::registry* world) {
  return WorldView(world, WorldView::Decl::Thin());
}

}  // namespace igecs
