#include <igasync/promise_combiner.h>
#include <igecs/scheduler.h>

#include <dglib/digraph.hh>
#include <dglib/digraphop.hh>
#include <dglib/schedule.hh>
#include <iostream>

namespace {
template <class T>
T& last(std::vector<T>& v) {
  return v[v.size() - 1];
}

}  // namespace

namespace igecs {

//
// Scheduler::Node::Builder
//
Scheduler::Node::Builder::Builder(Scheduler::Node::NodeId node_id,
                                  Scheduler::Builder& b)
    : node_id_(node_id), is_main_thread_only_(false), b_(b), is_built_(false) {}

Scheduler::Node::Builder& Scheduler::Node::Builder::main_thread_only() {
  is_main_thread_only_ = true;
  return *this;
}

Scheduler::Node::Builder& Scheduler::Node::Builder::with_decl(
    WorldView::Decl decl) {
  world_view_decl_.merge_in_decl(decl);
  return *this;
}

Scheduler::Node::Builder& Scheduler::Node::Builder::depends_on(
    const Scheduler::Node& node) {
  if (!::vec_contains(dependency_ids_, node.id_)) {
    dependency_ids_.push_back(node.id_);
    dependency_cttis_.push_back(node.system_id_);
  }
  return *this;
}

Scheduler::Node Scheduler::Node::Builder::build(
    std::function<std::shared_ptr<igasync::Promise<void>>(
        WorldView* wv, std::shared_ptr<igasync::TaskList> main_thread_task_list,
        std::shared_ptr<igasync::TaskList> any_thread_task_list,
        std::function<void(igasync::TaskProfile profile)> profile_cb)>
        cb,
    igecs::CttiTypeId system_id, std::string system_name) {
  assert(!is_built_ && "[IgECS::Scheduler] Node is already built");

  auto node =
      Scheduler::Node(std::move(world_view_decl_), is_main_thread_only_,
                      node_id_, std::move(dependency_ids_), std::move(cb),
                      system_id, system_name, dependency_cttis_);
  b_.nodes_.push_back(node);

  is_built_ = true;

  return node;
}

Scheduler::Node Scheduler::Node::Builder::build(void (*cb)(WorldView* wv),
                                                igecs::CttiTypeId system_id,
                                                std::string system_name) {
  auto fn = [cb](WorldView* wv,
                 std::shared_ptr<igasync::TaskList> main_thread_task_list,
                 std::shared_ptr<igasync::TaskList> any_thread_task_list,
                 std::function<void(igasync::TaskProfile profile)> profile_cb) {
    cb(wv);
    return igasync::Promise<void>::Immediate();
  };
  return build(std::move(fn), system_id, system_name);
}

//
// Scheduler::Node
//
Scheduler::Node::Node()
    : id_(NodeId{0}),
      system_id_(),
      main_thread_only_(false),
      wv_decl_(WorldView::Decl::Thin()) {}

Scheduler::Node::Node(
    WorldView::Decl wv_decl, bool main_thread_only, NodeId id,
    std::vector<NodeId> dependency_ids,
    std::function<std::shared_ptr<igasync::Promise<void>>(
        WorldView* wv, std::shared_ptr<igasync::TaskList> main_thread_task_list,
        std::shared_ptr<igasync::TaskList> any_thread_task_list,
        std::function<void(igasync::TaskProfile profile)> profile_cb)>
        cb,
    igecs::CttiTypeId system_id, std::string system_name,
    std::vector<igecs::CttiTypeId> dependency_cttis)
    : id_(id),
      main_thread_only_(main_thread_only),
      wv_decl_(std::move(wv_decl)),
      cb_(std::move(cb)),
      dependency_ids_(std::move(dependency_ids)),
      system_id_(system_id),
      system_name_(std::move(system_name)),
      dependency_cttis_(std::move(dependency_cttis)) {}

std::shared_ptr<igasync::Promise<void>> Scheduler::Node::schedule(
    entt::registry* world, std::shared_ptr<igasync::TaskList> main_thread,
    std::shared_ptr<igasync::TaskList> any_thread,
    profile::FrameProfiler* profiler) {
  auto system_id = system_id_;
  auto rsl = igasync::Promise<void>::Create();
  std::shared_ptr<igasync::TaskList> tl =
      main_thread_only_ ? main_thread : any_thread;

  auto profile_cb = [main_thread, profiler,
                     system_id](igasync::TaskProfile profile) {
    main_thread->schedule(igasync::Task::Of(
        [profiler, system_id](igasync::TaskProfile profile) {
          // TODO (sessamekesh): Number of entities accessed here
          //  (that should be handled in a igecs wv layer)
          profiler->AddExecution(system_id.id, profile.Started,
                                 profile.Finished, 0u,
                                 profile.ExecutorThreadId);
        },
        profile));
  };

  tl->schedule(igasync::Task::WithProfile(
      profile_cb, [this, world, rsl, main_thread, any_thread, profile_cb]() {
        auto wv = new igecs::WorldView(wv_decl_.create(world));
        cb_(wv, main_thread, any_thread, profile_cb)
            ->on_resolve(
                [rsl, wv]() {
                  rsl->resolve();
                  delete wv;
                },
                any_thread);
      }));

  return rsl;
}

//
// Scheduler::Builder
//
Scheduler::Builder::Builder(std::string graph_name)
    : graph_name_(graph_name),
      max_spin_time_(std::chrono::milliseconds(10)),
      next_node_id_(1ul) {}

Scheduler::Builder& Scheduler::Builder::max_spin_time(
    std::chrono::high_resolution_clock::duration dt) {
  max_spin_time_ = dt;
  return *this;
}

Scheduler::Builder& Scheduler::Builder::main_thread_id(std::thread::id id) {
  main_thread_id_ = id;
  return *this;
}

Scheduler::Builder& Scheduler::Builder::worker_thread_id(std::thread::id id) {
  for (int i = 0; i < worker_thread_ids_.size(); i++) {
    if (worker_thread_ids_[i] == id) {
      return *this;
    }
  }

  worker_thread_ids_.push_back(id);
  return *this;
}

Scheduler::Node::Builder Scheduler::Builder::add_node() {
  return Scheduler::Node::Builder(Scheduler::Node::NodeId{next_node_id_++},
                                  *this);
}

Scheduler Scheduler::Builder::build() { return Scheduler(*this); }

//
// Scheduler
//

bool Scheduler::has_strict_dep(const std::vector<Node>& nodes, Node::NodeId a,
                               Node::NodeId b) {
  auto find_node = [&nodes](Node::NodeId n) -> const Node& {
    for (int i = 0; i < nodes.size(); i++) {
      if (nodes[i].id_ == n) {
        return nodes[i];
      }
    }

    assert(false && "No node with id found");
    return nodes[0];
  };

  const Node& na = find_node(a);
  for (int i = 0; i < na.dependency_ids_.size(); i++) {
    if (na.dependency_ids_[i] == b ||
        has_strict_dep(nodes, na.dependency_ids_[i], b)) {
      return true;
    }
  }

  return false;
}

Scheduler::Scheduler(Scheduler::Builder b)
    : frame_profiler_(b.graph_name_, b.main_thread_id_,
                      std::move(b.worker_thread_ids_)),
      max_spin_time_(b.max_spin_time_) {
  // Build a digraph of nodes...
  digraph<Node::NodeId, int> g;
  for (int i = 0; i < b.nodes_.size(); i++) {
    g.add(b.nodes_[i].id_);
    for (int j = 0; j < b.nodes_[i].dependency_ids_.size(); j++) {
      g.add(b.nodes_[i].id_, b.nodes_[i].dependency_ids_[j], 1);
    }
  }
#ifdef IG_ENABLE_ECS_VALIDATION
  // Make sure that there are no cycles in the graph (using digraph lib)
  assert(cycles(g) == 0 && "[IgECS::Scheduler] Cycle in input graph found!");

  // Make sure all nodes that write any component writes either strictly depend
  //  on, or are depended on by, all other nodes that read/write that same
  //  component type in the same context
  for (int node_idx = 0; node_idx < b.nodes_.size(); node_idx++) {
    const auto& node = b.nodes_[node_idx];
    const auto& ctx_writes = node.wv_decl_.list_ctx_writes();
    const auto& writes = node.wv_decl_.list_writes();
    const auto& consumes = node.wv_decl_.list_evt_consumes();

    // CTX read/write comparisons...
    for (int ctx_write_idx = 0; ctx_write_idx < ctx_writes.size();
         ctx_write_idx++) {
      const auto& ctx_write = ctx_writes[ctx_write_idx];

      for (int compare_node_idx = 0; compare_node_idx < b.nodes_.size();
           compare_node_idx++) {
        if (compare_node_idx == node_idx) continue;

        const auto& compare_node = b.nodes_[compare_node_idx];
        if (::vec_contains(compare_node.wv_decl_.list_ctx_writes(),
                           ctx_write) ||
            ::vec_contains(compare_node.wv_decl_.list_ctx_reads(), ctx_write)) {
          if (!has_strict_dep(b.nodes_, node.id_, compare_node.id_) &&
              !has_strict_dep(b.nodes_, compare_node.id_, node.id_)) {
            assert(false &&
                   "[IgECS::Scheduler] Strict dependency not found between "
                   "ctx_write and other ctx access!");
          }
        }
      }
    }

    // Component read/write comparisons...
    for (int write_idx = 0; write_idx < writes.size(); write_idx++) {
      const auto& write = writes[write_idx];

      for (int compare_node_idx = 0; compare_node_idx < b.nodes_.size();
           compare_node_idx++) {
        if (compare_node_idx == node_idx) continue;

        const auto& compare_node = b.nodes_[compare_node_idx];
        if (::vec_contains(compare_node.wv_decl_.list_writes(), write) ||
            ::vec_contains(compare_node.wv_decl_.list_reads(), write)) {
          if (!has_strict_dep(b.nodes_, node.id_, compare_node.id_) &&
              !has_strict_dep(b.nodes_, compare_node.id_, node.id_)) {
            assert(false &&
                   "[IgECS::Scheduler] Strict dependency not found between "
                   "write and other component access!");
          }
        }
      }
    }

    // Event queue/consume comparisons...
    for (int consume_idx = 0; consume_idx < consumes.size(); consume_idx++) {
      const auto& consume = consumes[consume_idx];

      for (int compare_node_idx = 0; compare_node_idx < b.nodes_.size();
           compare_node_idx++) {
        if (compare_node_idx == node_idx) continue;

        const auto& compare_node = b.nodes_[compare_node_idx];
        if (::vec_contains(compare_node.wv_decl_.list_evt_writes(), consume) ||
            ::vec_contains(compare_node.wv_decl_.list_evt_consumes(),
                           consume)) {
          if (!has_strict_dep(b.nodes_, node.id_, compare_node.id_) &&
              !has_strict_dep(b.nodes_, compare_node.id_, node.id_)) {
            assert(false &&
                   "[IgECS::Scheduler] Strict dependency not found between "
                   "event consume and event enqueue nodes!");
          }
        }
      }
    }
  }
#endif
  auto s = bfschedule(g);
  for (Node::NodeId nid : s.elements()) {
#ifdef IG_ENABLE_ECS_VALIDATION
    bool found_node = false;
#endif
    for (int i = 0; i < b.nodes_.size(); i++) {
      if (b.nodes_[i].id_ == nid) {
        nodes_.push_back(b.nodes_[i]);
        frame_profiler_.AddSystem(
            b.nodes_[i].system_id_, b.nodes_[i].system_name_,
            b.nodes_[i].wv_decl_, b.nodes_[i].main_thread_only_);
        for (auto& dep : b.nodes_[i].dependency_cttis_) {
          frame_profiler_.AddSystemDependency(b.nodes_[i].system_id_, dep);
        }
#ifdef IG_ENABLE_ECS_VALIDATION
        found_node = true;
#endif
        break;
      }
    }
#ifdef IG_ENABLE_ECS_VALIDATION
    assert(found_node &&
           "[IgECS::Scheduler] Input node with given ID not found!");
#endif
  }

  if (nodes_.size() == 0) {
    std::cerr << "[IgECS::Scheduler] Degenerate schedule (size=0) created"
              << std::endl;
  }
}

void Scheduler::execute(std::shared_ptr<igasync::TaskList> any_thread_task_list,
                        entt::registry* world) {
  frame_profiler_.StartFrame();

  // Degenerate case
  if (nodes_.size() == 0) {
    return;
  }

  auto main_thread_task_list = igasync::TaskList::Create();

  // Single threaded case: concurrency is not real, and all tasks should be
  //  scheduled against the main thread
  if (any_thread_task_list == nullptr) {
    any_thread_task_list = main_thread_task_list;
  }

  struct SNode {
    Node::NodeId id;
    std::shared_ptr<igasync::Promise<void>> rslPromise;
  };
  std::vector<SNode> schedule;
  schedule.reserve(nodes_.size());

  auto all_nodes_combiner = igasync::PromiseCombiner::Create();

  for (int i = 0; i < nodes_.size(); i++) {
    if (nodes_[i].dependency_ids_.size() == 0) {
      schedule.push_back(SNode{
          nodes_[i].id_,
          nodes_[i].schedule(world, main_thread_task_list, any_thread_task_list,
                             &frame_profiler_),
      });
      all_nodes_combiner->add(::last(schedule).rslPromise,
                              any_thread_task_list);
      continue;
    }

    auto node_combiner = igasync::PromiseCombiner::Create();

    for (int j = 0; j < nodes_[i].dependency_ids_.size(); j++) {
#ifdef IG_ENABLE_ECS_VALIDATION
      bool found_dep = false;
#endif
      for (int nid = 0; nid < schedule.size(); nid++) {
        const auto& sn = schedule[nid];
        if (sn.id == nodes_[i].dependency_ids_[j]) {
          node_combiner->add(sn.rslPromise, any_thread_task_list);
#ifdef IG_ENABLE_ECS_VALIDATION
          found_dep = true;
#endif
          break;
        }
      }
#ifdef IG_ENABLE_ECS_VALIDATION
      assert(found_dep &&
             "[IgECS::Scheduler] Node dependency listed but not found");
#endif
    }

    schedule.push_back(SNode{
        nodes_[i].id_,
        node_combiner->combine([](auto) {}, any_thread_task_list)
            ->then_chain(
                [this, i, world, main_thread_task_list, any_thread_task_list] {
                  return this->nodes_[i].schedule(world, main_thread_task_list,
                                                  any_thread_task_list,
                                                  &frame_profiler_);
                },
                main_thread_task_list),
    });
    all_nodes_combiner->add(::last(schedule).rslPromise, any_thread_task_list);
  }

  // Schedule is built! Proceed to execute everything...
  bool is_done = false;
  all_nodes_combiner->combine([&is_done](auto) { is_done = true; },
                              main_thread_task_list);

  //
  // Okay this is a tricky section full of weird shit.
  //
  // Basically: follow this sequence of events:
  // (1) Spin, doing everything that can be done against main_thread list
  // (2) Once the main thread is waiting, pull something from any_thread list
  // (3) Repeat so long as either main or any has any work to do
  // (4) If any work was done, clear any existing hanging state.
  // (5) If no work was done, set a hanging state if one is not found.
  // (6) If a hanging state is present and stale for longer than max_spin_time_,
  //     trigger an assertion failure that crashes the application (this helps
  //     guard against infinite spinloops in scheduling in case of bugs)
  std::optional<std::chrono::high_resolution_clock::time_point> hang_start = {};
  while (!is_done) {
    bool did_a_thing = false;
    bool is_spinning = true;
    do {
      did_a_thing = false;
      while (main_thread_task_list->execute_next()) {
        did_a_thing = true;
      }
      if (!did_a_thing) {
        did_a_thing = did_a_thing || any_thread_task_list->execute_next();
      }
      if (did_a_thing) {
        is_spinning = false;
      }
    } while (did_a_thing);

    if (!is_spinning) {
      if (hang_start.has_value()) {
        // TODO (sessamekesh): telemetry around unnecessary hang time (if any)
        hang_start = {};
      }
    } else {
      // Hanging!
      if (!hang_start.has_value()) {
        hang_start = std::chrono::high_resolution_clock::now();
      } else {
        if ((*hang_start + max_spin_time_) <
            std::chrono::high_resolution_clock::now()) {
          // Max spin elapsed - fail spectacularly
          assert(false &&
                 "[IgECS::Scheduler] Maximum spin time elapsed - game must now "
                 "crash");
        }
      }
    }
  }

  // Flush main thread task list to capture any lingering tasks (profiles etc)
  while (main_thread_task_list->execute_next()) {
  }

  frame_profiler_.EndFrame();
}

std::string Scheduler::dump_profile(bool pretty) {
  return frame_profiler_.JsonSerializeFrame(pretty);
}

}  // namespace igecs
