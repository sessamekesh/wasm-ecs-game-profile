#ifndef IGECS_CTTI_TYPE_ID_H
#define IGECS_CTTI_TYPE_ID_H

#include <igecs/config.h>

#include <atomic>
#include <string>

namespace {
std::string trim_type_name_msvc(const char* tn) {
  const size_t off = 123;
  const size_t tail = 7;

  std::string stn(tn);
  std::string w_class_or_struct = stn.substr(off, stn.length() - off - tail);

  if (w_class_or_struct.find("struct") == 0) {
    return w_class_or_struct.substr(7);
  }
  return w_class_or_struct.substr(6);
}

std::string trim_type_name_clang(const char* tn) {
  const size_t off = 58;
  const size_t tail = 1;

  std::string stn(tn);
  return stn.substr(off, stn.length() - off - tail);
}

}  // namespace

namespace igecs {

struct CttiTypeId {
 private:
  static inline std::string empty_str = "";

 public:
  uint32_t id;
  const std::string& name;

  CttiTypeId() : id(0), name(CttiTypeId::empty_str) {}
  CttiTypeId(uint32_t _id, const std::string& _name) : id(_id), name(_name) {}

  bool operator==(const CttiTypeId& o) const { return id == o.id; }
  bool operator!=(const CttiTypeId& o) const { return id != o.id; }
  bool operator<(const CttiTypeId& o) const { return id < o.id; }

  template <typename T>
  static CttiTypeId of() {
    static uint32_t tid = next_type_id_++;
    static std::string name = CttiTypeId::GetName<T>();
    return CttiTypeId{tid, name};
  }

  template <typename T>
  static std::string GetName() {
#ifdef IG_ENABLE_ECS_VALIDATION
#ifdef _MSC_VER
    return trim_type_name_msvc(__FUNCSIG__);
#elif __clang__
    return trim_type_name_clang(__PRETTY_FUNCTION__);
#else
#error \
    "CTTI pretty names not supported by this compiler - disable IG_ENABLE_ECS_VALIDATION"
#endif
#else
    // If no validation is built in, just return something silly like this (at
    // least it's uniquely identifying)
    return std::string("T-") + std::to_string(CttiTypeId::of<T>().id);
#endif
  }

 private:
  static std::atomic_uint32_t next_type_id_;
};

}  // namespace igecs

#endif