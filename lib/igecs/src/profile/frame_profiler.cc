#include <igecs/profile/frame_profiler.h>

#include <nlohmann/json.hpp>

namespace igecs::profile {

FrameProfiler::FrameProfiler(
    std::string graph_name, std::thread::id main_thread_id,
    const std::vector<std::thread::id>& worker_thread_ids)
    : graph_name_(graph_name),
      main_thread_id_(main_thread_id),
      worker_thread_ids_(worker_thread_ids) {}

void FrameProfiler::AddSystem(igecs::CttiTypeId system_ctti,
                              std::string system_name,
                              const igecs::WorldView::Decl& access,
                              bool main_thread_only) {
  RegisteredSystem system{};

  system.system_id = system_ctti.id;
  system.system_name = system_name;
  system.main_thread_only = main_thread_only;

  for (auto& ctx_read : access.list_ctx_reads()) {
    system.component_access.push_back(
        {ComponentAccessMode::CtxRead, ctx_read.id});
    components_[ctx_read.id] = RegisteredComponent{ctx_read.id, ctx_read.name};
  }
  for (auto& ctx_write : access.list_ctx_writes()) {
    system.component_access.push_back(
        {ComponentAccessMode::CtxWrite, ctx_write.id});
    components_[ctx_write.id] =
        RegisteredComponent{ctx_write.id, ctx_write.name};
  }
  for (auto& read : access.list_reads()) {
    system.component_access.push_back(
        {ComponentAccessMode::ComponentRead, read.id});
    components_[read.id] = RegisteredComponent{read.id, read.name};
  }
  for (auto& write : access.list_writes()) {
    system.component_access.push_back(
        {ComponentAccessMode::ComponentWrite, write.id});
    components_[write.id] = RegisteredComponent{write.id, write.name};
  }
  for (auto& list_write : access.list_evt_writes()) {
    system.component_access.push_back(
        {ComponentAccessMode::ListWrite, list_write.id});
    components_[list_write.id] =
        RegisteredComponent{list_write.id, list_write.name};
  }
  for (auto& list_consume : access.list_evt_consumes()) {
    system.component_access.push_back(
        {ComponentAccessMode::ListConsume, list_consume.id});
    components_[list_consume.id] =
        RegisteredComponent{list_consume.id, list_consume.name};
  }

  systems_.push_back(system);
}

void FrameProfiler::AddSystemDependency(igecs::CttiTypeId system_ctti,
                                        igecs::CttiTypeId dep_id) {
  for (auto& system : systems_) {
    if (system.system_id == system_ctti.id) {
      system.dependencies.push_back(dep_id.id);
    }
  }
}

void FrameProfiler::ClearSystemRegistry() { systems_.clear(); }

void FrameProfiler::StartFrame() {
  executions_.clear();
  frame_start_ = std::chrono::high_resolution_clock::now();
  frame_end_ = std::chrono::high_resolution_clock::time_point::min();
}

void FrameProfiler::EndFrame() {
  frame_end_ = std::chrono::high_resolution_clock::now();
}

void FrameProfiler::AddExecution(
    std::uint16_t system_id,
    std::chrono::high_resolution_clock::time_point start_time,
    std::chrono::high_resolution_clock::time_point end_time,
    std::uint32_t entities_accessed, std::thread::id thread_id) {
  SystemRun execution{};
  execution.system_id = system_id;
  execution.thread_id = thread_id;
  execution.entities_accessed = entities_accessed;
  execution.start_frame_time = start_time;
  execution.end_frame_time = end_time;

  executions_.push_back(execution);
}

std::string FrameProfiler::JsonSerializeFrame(bool pretty) {
  static auto thread_id_hasher = std::hash<std::thread::id>();

  static auto stringify_component_access =
      [](FrameProfiler::ComponentAccessMode mode) -> const char* {
    switch (mode) {
      case FrameProfiler::ComponentAccessMode::CtxRead:
        return "CtxRead";
      case FrameProfiler::ComponentAccessMode::CtxWrite:
        return "CtxWrite";
      case FrameProfiler::ComponentAccessMode::ComponentRead:
        return "ComponentRead";
      case FrameProfiler::ComponentAccessMode::ComponentWrite:
        return "ComponentWrite";
      case FrameProfiler::ComponentAccessMode::ListWrite:
        return "ListWrite";
      case FrameProfiler::ComponentAccessMode::ListConsume:
        return "ListConsume";
      default:
        return "<< access_unknown >>";
    }
  };

  nlohmann::json j;

  nlohmann::json systemsj = nlohmann::json::array();
  for (auto& system : systems_) {
    nlohmann::json systemj = {{"id", system.system_id},
                              {"name", system.system_name},
                              {"dependencies", system.dependencies}};
    if (system.main_thread_only) {
      systemj["main_thread"] = true;
    }

    nlohmann::json accessj = nlohmann::json::array();
    for (auto& access : system.component_access) {
      accessj.push_back(
          {{"mode", stringify_component_access(access.access_mode)},
           {"component_id", access.component_id}});
    }
    systemj["access"] = accessj;

    systemsj.push_back(systemj);
  }
  j["systems"] = systemsj;

  nlohmann::json componentsj = nlohmann::json::array();
  for (auto& component : components_) {
    componentsj.push_back({{"id", component.second.component_id},
                           {"name", component.second.component_name}});
  }
  j["components"] = componentsj;

  j["frame_meta"]["main_thread_id"] = thread_id_hasher(main_thread_id_);
  j["frame_meta"]["worker_thread_ids"] = nlohmann::json::array();
  for (auto& worker_thread_id : worker_thread_ids_) {
    j["frame_meta"]["worker_thread_ids"].push_back(
        thread_id_hasher(worker_thread_id));
  }
  j["frame_meta"]["total_time"] =
      std::chrono::duration_cast<std::chrono::microseconds>(frame_end_ -
                                                            frame_start_)
          .count();
  j["frame_meta"]["graph_name"] = graph_name_;

  auto exj = nlohmann::json::array();
  for (auto& ex : executions_) {
    auto start_time = std::chrono::duration_cast<std::chrono::microseconds>(
                          ex.start_frame_time - frame_start_)
                          .count();
    auto end_time = std::chrono::duration_cast<std::chrono::microseconds>(
                        ex.end_frame_time - frame_start_)
                        .count();
    exj.push_back({{"system_id", ex.system_id},
                   {"thread_id", thread_id_hasher(ex.thread_id)},
                   {"entities_accessed", ex.entities_accessed},
                   {"start_time", start_time},
                   {"end_time", end_time}});
  }

  j["executions"] = exj;

  if (pretty) {
    return j.dump(4);
  }

  return j.dump();
}

}  // namespace igecs::profile