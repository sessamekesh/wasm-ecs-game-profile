#ifndef IGECS_PROFILE_FRAME_PROFILER_H
#define IGECS_PROFILE_FRAME_PROFILER_H

#include <igecs/world_view.h>

#include <chrono>
#include <vector>
#include <map>

namespace igecs::profile {

class FrameProfiler {
 public:
  FrameProfiler(std::string graph_name, std::thread::id main_thread_id,
                const std::vector<std::thread::id>& worker_thread_ids);

  //
  // Registry
  //
  void AddSystem(igecs::CttiTypeId system_ctti, std::string system_name,
                 const igecs::WorldView::Decl& access, bool main_thread_only);
  void AddSystemDependency(igecs::CttiTypeId system_ctti,
                           igecs::CttiTypeId dep_id);

  void ClearSystemRegistry();

  //
  // Frame
  //
  void StartFrame();
  void EndFrame();
  void AddExecution(std::uint16_t system_id,
                    std::chrono::high_resolution_clock::time_point start_time,
                    std::chrono::high_resolution_clock::time_point end_time,
                    std::uint32_t entities_accessed, std::thread::id thread_id);
  std::string JsonSerializeFrame(bool pretty);

 private:
  enum class ComponentAccessMode {
    CtxRead,
    CtxWrite,
    ComponentRead,
    ComponentWrite,
    ListWrite,
    ListConsume
  };
  struct ComponentAccess {
    ComponentAccessMode access_mode;
    std::uint32_t component_id;
  };
  struct RegisteredSystem {
    std::uint32_t system_id;
    std::string system_name;
    std::vector<std::uint32_t> dependencies;
    std::vector<ComponentAccess> component_access;
    bool main_thread_only;
  };
  struct RegisteredComponent {
    std::uint32_t component_id;
    std::string component_name;
  };
  struct SystemRun {
    std::uint32_t system_id;
    std::thread::id thread_id;
    std::uint32_t entities_accessed;
    std::chrono::high_resolution_clock::time_point start_frame_time;
    std::chrono::high_resolution_clock::time_point end_frame_time;
  };

 private:
  // Graph metadata
  std::string graph_name_;
  std::thread::id main_thread_id_;
  std::vector<std::thread::id> worker_thread_ids_;
  std::vector<RegisteredSystem> systems_;
  std::map<std::uint32_t, RegisteredComponent> components_;

  // Frame data
  std::chrono::high_resolution_clock::time_point frame_start_;
  std::chrono::high_resolution_clock::time_point frame_end_;
  std::vector<SystemRun> executions_;
};

}  // namespace igecs::profile

#endif
