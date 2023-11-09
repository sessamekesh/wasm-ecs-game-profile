#ifndef IGECS_WORLD_VIEW_H
#define IGECS_WORLD_VIEW_H

#include <igecs/config.h>
#include <igecs/ctti_type_id.h>

#include "evt_queue.h"

#ifdef IG_ENABLE_ECS_VALIDATION
#include <iostream>
#include <set>
#endif

#include <entt/entt.hpp>

namespace {

template <typename T>
T& get_ctx_or_create_default(entt::registry& r) {
  if (r.ctx().contains<T>()) {
    return r.ctx().get<T>();
  }

  r.ctx().emplace<T>();
  return r.ctx().get<T>();
}

#ifdef IG_ENABLE_ECS_VALIDATION
template <typename T>
bool vec_contains(const std::vector<T>& v, const T& ele) {
  return std::find(v.begin(), v.end(), ele) != v.end();
}

template <typename T>
std::string failmsg(const char* method) {
  return (std::string("ECS validation failure: method ") + method +
          std::string(" failed for type ") + igecs::CttiTypeId::GetName<T>());
}

template <typename T>
void assert_and_print(bool condition, const char* method) {
  if (!condition) {
    std::cerr << ::failmsg<T>(method) << std::endl;
  }
  assert(condition);
}
#endif
}  // namespace

namespace igecs {
class WorldView {
 public:
  class Decl {
   public:
    Decl();
    Decl(const Decl&) = default;
    Decl(Decl&&) = default;
    Decl& operator=(const Decl&) = default;
    Decl& operator=(Decl&&) = default;

    static Decl Thin();

    Decl& merge_in_decl(const Decl& o);

    template <typename T>
    Decl& reads() {
#ifdef IG_ENABLE_ECS_VALIDATION
      reads_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    Decl& writes() {
#ifdef IG_ENABLE_ECS_VALIDATION
      writes_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
      reads_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    Decl& ctx_reads() {
#ifdef IG_ENABLE_ECS_VALIDATION
      ctx_reads_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    Decl& ctx_writes() {
#ifdef IG_ENABLE_ECS_VALIDATION
      ctx_reads_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
      ctx_writes_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    Decl& evt_writes() {
#ifdef IG_ENABLE_ECS_VALIDATION
      evt_writes_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    Decl& evt_consumes() {
#ifdef IG_ENABLE_ECS_VALIDATION
      evt_consumes_.push_back(CttiTypeId::of<std::remove_const_t<T>>());
#endif
      return *this;
    }

    template <typename T>
    [[nodiscard]] bool can_read() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             ::vec_contains(reads_, CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    template <typename T>
    [[nodiscard]] bool can_write() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             ::vec_contains(writes_, CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    template <typename T>
    [[nodiscard]] bool can_ctx_read() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             ::vec_contains(ctx_reads_,
                            CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    template <typename T>
    [[nodiscard]] bool can_ctx_write() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             ::vec_contains(ctx_writes_,
                            CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    template <typename T>
    [[nodiscard]] bool can_evt_write() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             ::vec_contains(evt_writes_,
                            CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    template <typename T>
    [[nodiscard]] bool can_evt_consume() const {
#ifdef IG_ENABLE_ECS_VALIDATION
      return allow_all_ ||
             ::vec_contains(evt_consumes_,
                            CttiTypeId::of<std::remove_const_t<T>>());
#else
      return true;
#endif
    }

    WorldView create(entt::registry* registry) const;

#ifdef IG_ENABLE_ECS_VALIDATION
    const std::vector<CttiTypeId>& list_reads() const { return reads_; }
    const std::vector<CttiTypeId>& list_writes() const { return writes_; }
    const std::vector<CttiTypeId>& list_ctx_reads() const { return ctx_reads_; }
    const std::vector<CttiTypeId>& list_ctx_writes() const {
      return ctx_writes_;
    }
    const std::vector<CttiTypeId>& list_evt_writes() const {
      return evt_writes_;
    }
    const std::vector<CttiTypeId>& list_evt_consumes() const {
      return evt_consumes_;
    }
#endif

   private:
    Decl(bool allow_all);

    bool allow_all_;
#ifdef IG_ENABLE_ECS_VALIDATION
    std::vector<CttiTypeId> reads_;
    std::vector<CttiTypeId> writes_;
    std::vector<CttiTypeId> ctx_reads_;
    std::vector<CttiTypeId> ctx_writes_;
    std::vector<CttiTypeId> evt_writes_;
    std::vector<CttiTypeId> evt_consumes_;
#endif
  };

 private:
  // Base case
  template <int = 0>
  bool view_test() const {
    return true;
  }

  template <
      typename T, typename... Others,
      typename std::enable_if<std::is_const<T>::value, int>::type* = nullptr>
  bool view_test() const {
#ifdef IG_ENABLE_ECS_VALIDATION
    if (!decl_.can_read<T>()) {
      std::cerr << "[WorldView] MUTABLE view_test failed for type "
                << CttiTypeId::GetName<T>() << std::endl;
      return false;
    }
    return view_test<Others...>();
    read_types_.insert(CttiTypeId::of<T>());
#endif
    return true;
  }

  template <
      typename T, typename... Others,
      typename std::enable_if<!std::is_const<T>::value, int>::type* = nullptr>
  bool view_test() const {
#ifdef IG_ENABLE_ECS_VALIDATION
    if (!decl_.can_write<T>()) {
      std::cerr << "[WorldView] IMMUTABLE view_test failed for type "
                << CttiTypeId::GetName<T>() << std::endl;
      return false;
    }
    return view_test<Others...>();
    write_types_.insert(CttiTypeId::of<T>());
#endif
    return true;
  }

 public:
  WorldView(entt::registry* registry, Decl decl);
  static WorldView Thin(entt::registry* registry);

#ifdef IG_ENABLE_ECS_VALIDATION
  template <typename T>
  bool can_read() {
    return decl_.can_read<T>();
  }

  template <typename T>
  bool can_write() {
    return decl_.can_write<T>();
  }

  template <typename T>
  bool can_ctx_read() {
    return decl_.can_ctx_read<T>();
  }

  template <typename T>
  bool can_ctx_write() {
    return decl_.can_ctx_write<T>();
  }

  template <typename T>
  bool can_enqueue_event() {
    return decl_.can_evt_write<T>();
  }

  template <typename T>
  bool can_consume_events() {
    return decl_.can_evt_consume<T>();
  }

 private:
  mutable std::set<CttiTypeId> read_types_;
  mutable std::set<CttiTypeId> write_types_;
  mutable std::set<CttiTypeId> ctx_read_types_;
  mutable std::set<CttiTypeId> ctx_write_types_;
  mutable std::set<CttiTypeId> evt_queue_types_;
  mutable std::set<CttiTypeId> evt_consume_types_;

 public:
  template <typename T>
  bool has_read() const {
    return read_types_.count(CttiTypeId::of<T>()) > 0;
  }
  template <typename T>
  bool has_written() const {
    return write_types_.count(CttiTypeId::of<T>()) > 0;
  }
  template <typename T>
  bool has_ctx_read() const {
    return ctx_read_types_.count(CttiTypeId::of<T>()) > 0;
  }
  template <typename T>
  bool has_ctx_written() const {
    return ctx_write_types_.count(CttiTypeId::of<T>()) > 0;
  }
  template <typename T>
  bool has_evt_enqueued() const {
    return evt_queue_types_.count(CttiTypeId::of<T>()) > 0;
  }
  template <typename T>
  bool has_evt_consumed() const {
    return evt_consume_types_.count(CttiTypeId::of<T>()) > 0;
  }
#endif

  template <typename T>
  T& mut_ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_write<T>(), "mut_ctx");
    ctx_write_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->ctx().get<T>();
  }

  template <typename T>
  const T& ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_read<T>(), "ctx");
    ctx_read_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->ctx().get<T>();
  }

  template <typename T, typename... Args>
  T& mut_ctx_or_set(Args&&... args) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_write<T>(), "mut_ctx_or_set");
    ctx_write_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->ctx().insert_or_assign<T, Args...>(
        std::forward<Args>(args)...);
  }

  template <typename T>
  bool has(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_read<T>(), "has");
    read_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->ctx().contains<T>(e);
  }

  template <typename T>
  bool ctx_has() const {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_read<T>(), "ctx_has");
    ctx_read_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->ctx().contains<T>();
  }

  template <typename T>
  T& write(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_write<T>(), "write");
    write_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->get<T>(e);
  }

  template <typename ComponentT, typename... Args>
  ComponentT& attach(entt::entity e, Args&&... args) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<ComponentT>(decl_.can_write<ComponentT>(), "attach");
    write_types_.insert(CttiTypeId::of<ComponentT>());
#endif
    return registry_->emplace<ComponentT, Args...>(e,
                                                   std::forward<Args>(args)...);
  }

  template <typename ComponentT, typename... Args>
  ComponentT& attach_or_replace(entt::entity e, Args&&... args) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<ComponentT>(decl_.can_write<ComponentT>(), "attach");
    write_types_.insert(CttiTypeId::of<ComponentT>());
#endif
    return registry_->emplace_or_replace<ComponentT, Args...>(
        e, std::forward<Args>(args)...);
  }

  template <typename ComponentT, typename... Args>
  ComponentT& attach_ctx(Args&&... args) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<ComponentT>(decl_.can_ctx_write<ComponentT>(),
                                   "attach_ctx");
    ctx_write_types_.insert(CttiTypeId::of<ComponentT>());
#endif
    return registry_->ctx().emplace<ComponentT, Args...>(
        std::forward<Args>(args)...);
  }

  template <typename T>
  const T& read(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_read<T>(), "read");
    read_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->get<T>(e);
  }

  template <typename T>
  size_t remove(entt::entity e) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_write<T>(), "remove");
    write_types_.insert(CttiTypeId::of<T>());
#endif
    return registry_->remove<T>(e);
  }

  template <typename T>
  void remove_ctx() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_ctx_write<T>(), "remove_ctx");
    ctx_write_types_.insert(CttiTypeId::of<T>());
#endif
    registry_->ctx().erase<T>();
  }

  template <typename Component, typename... Other, typename... Exclude>
  auto view(entt::exclude_t<Exclude...> e = entt::exclude_t{}) {
#ifdef IG_ENABLE_ECS_VALIDATION
    bool rsl = view_test<Component, Other...>();
    assert(rsl);
#endif
    return registry_->view<Component, Other..., Exclude...>(e);
  }

  template <typename T>
  void enqueue_event(T&& evt) {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_evt_write<T>(), "enqueue_event");
    evt_queue_types_.insert(CttiTypeId::of<T>());
#endif
    CtxEventQueue<T>& ctx_queue =
        ::get_ctx_or_create_default<CtxEventQueue<T>>(*registry_);
    ctx_queue.queue.enqueue(evt);
  }

  template <typename T>
  std::vector<T> consume_events() {
#ifdef IG_ENABLE_ECS_VALIDATION
    ::assert_and_print<T>(decl_.can_evt_consume<T>(), "consume_events");
    evt_consume_types_.insert(CttiTypeId::of<T>());
#endif
    CtxEventQueue<T>& ctx_queue =
        ::get_ctx_or_create_default<CtxEventQueue<T>>(*registry_);
    T bulk_events[16]{};
    size_t num_evts = 0;
    std::vector<T> evts;
    while ((num_evts = ctx_queue.queue.try_dequeue_bulk(bulk_events, 16)) !=
           0) {
      for (size_t i = 0; i < num_evts; i++) {
        evts.push_back(std::move(bulk_events[i]));
      }
    }
    return std::move(evts);
  }

  inline entt::entity create() { return registry_->create(); }

  inline bool valid(entt::entity e) { return registry_->valid(e); }

 private:
  entt::registry* registry_;
  Decl decl_;
};
}  // namespace igecs

#endif
