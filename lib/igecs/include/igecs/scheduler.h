#ifndef IGECS_SCHEDULER_H
#define IGECS_SCHEDULER_H

#include <igasync/promise.h>
#include <igasync/task_list.h>
#include <igecs/profile/frame_profiler.h>
#include <igecs/world_view.h>

#include <chrono>
#include <ostream>
#include <vector>

namespace igecs {

namespace concepts {
template <typename T>
concept HasSynchronousUpdateMethod = requires(T&& t, igecs::WorldView* wv) {
  { T::update(wv) } -> std::same_as<void>;
};

template <typename T>
concept HasSynchronousRunMethod = requires(T&& t, igecs::WorldView* wv) {
  { T::run(wv) } -> std::same_as<void>;
};
}  // namespace concepts

/**
 * ECS scheduler class - used to create a schedule graph and perform execution
 *  of ECS systems concurrently.
 */
class Scheduler {
 public:
  class Builder;

  /** Individual node on the scheduler object */
  class Node {
   public:
    friend class Scheduler;

    Node();

    struct NodeId {
      uint32_t id;

      bool operator==(const NodeId& o) const { return id == o.id; }
      bool operator<(const NodeId& o) const { return id < o.id; }
    };

    class Builder {
     public:
      friend class Node;
      friend class Scheduler;
      friend class Builder;

      Builder& main_thread_only();
      Builder& with_decl(WorldView::Decl decl);
      Builder& depends_on(const Node& node);

      /** Callback consumes a WorldView, and returns an EmptyPromiseRsl */
      [[nodiscard]] Node build(
          std::function<std::shared_ptr<igasync::Promise<void>>(
              WorldView* wv,
              std::function<void(igasync::TaskProfile profile)> profile_cb)>
              cb,
          igecs::CttiTypeId system_id, std::string system_name);

      /** Shorthand for use with synchronous systems */
      [[nodiscard]] Node build(void (*cb)(WorldView* wv),
                               igecs::CttiTypeId system_id,
                               std::string system_name);

      template <typename SystemT>
        requires(concepts::HasSynchronousRunMethod<SystemT>)
      [[nodiscard]] Node build() {
        return build(SystemT::run, igecs::CttiTypeId::of<SystemT>(),
                     igecs::CttiTypeId::GetName<SystemT>());
      }

      template <typename SystemT>
        requires(concepts::HasSynchronousUpdateMethod<SystemT>)
      [[nodiscard]] Node build() {
        return build(SystemT::update, igecs::CttiTypeId::of<SystemT>(),
                     igecs::CttiTypeId::GetName<SystemT>());
      }

     private:
      Builder(NodeId node_id, Scheduler::Builder& b);

      bool is_built_;
      NodeId node_id_;
      bool is_main_thread_only_;
      WorldView::Decl world_view_decl_;
      std::vector<NodeId> dependency_ids_;

      std::vector<igecs::CttiTypeId> dependency_cttis_;

      Scheduler::Builder& b_;
    };

    std::shared_ptr<igasync::Promise<void>> schedule(
        entt::registry* world,
        std::shared_ptr<igasync::ExecutionContext> main_thread,
        std::shared_ptr<igasync::ExecutionContext> any_thread,
        profile::FrameProfiler* profiler);

   private:
    Node(WorldView::Decl wv_decl, bool main_thread_only, NodeId id,
         std::vector<NodeId> dependency_ids,
         std::function<std::shared_ptr<igasync::Promise<void>>(
             WorldView* wv,
             std::function<void(igasync::TaskProfile profile)> profile_cb)>
             cb,
         igecs::CttiTypeId system_id, std::string system_name,
         std::vector<igecs::CttiTypeId> dependency_cttis);

    NodeId id_;
    bool main_thread_only_;
    WorldView::Decl wv_decl_;
    std::function<std::shared_ptr<igasync::Promise<void>>(
        WorldView* wv,
        std::function<void(igasync::TaskProfile profile)> profile_cb)>
        cb_;
    std::vector<NodeId> dependency_ids_;

    igecs::CttiTypeId system_id_;
    std::string system_name_;
    std::vector<igecs::CttiTypeId> dependency_cttis_;
  };

  /** Builder for the scheduler */
  class Builder {
   public:
    friend class Scheduler;
    friend class Node;
    friend class Builder;
    Builder(std::string graph_name);

    /** What is the longest time spinning is allowed before crashing the app? */
    Builder& max_spin_time(std::chrono::high_resolution_clock::duration dt);
    Builder& main_thread_id(std::thread::id id);
    Builder& worker_thread_id(std::thread::id id);

    [[nodiscard]] Node::Builder add_node();
    [[nodiscard]] Scheduler build();

   private:
    std::string graph_name_;
    std::thread::id main_thread_id_;
    std::vector<std::thread::id> worker_thread_ids_;
    std::vector<Node> nodes_;
    std::chrono::high_resolution_clock::duration max_spin_time_;
    uint32_t next_node_id_;
  };

  void execute(std::shared_ptr<igasync::TaskList> any_thread_task_list,
               entt::registry* world);

  std::string dump_profile(bool pretty = true);

 private:
  Scheduler(Builder b);

  /** Return true if and only if a eventually depends on b */
  static bool has_strict_dep(const std::vector<Node>& nodes, Node::NodeId a,
                             Node::NodeId b);

  profile::FrameProfiler frame_profiler_;
  std::chrono::high_resolution_clock::duration max_spin_time_;
  std::vector<Node> nodes_;

  // TODO (sessamekesh): Store nodes in a DAG for convenience in scheduling
};

}  // namespace igecs

#endif
