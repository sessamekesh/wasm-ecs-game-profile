set(igasset_headers
  "include/igasset/draco_dec.h"
  "include/igasset/igpack_decoder.h"
  "include/igasset/ozz_jobs.h"
  "include/igasset/ozz_wrappers.h"
  "include/igasset/vertex_types.h")

set(igasset_sources
  "src/draco_dec.cc"
  "src/igpack_decoder.cc"
  "src/ozz_jobs.cc")

gen_flatbuffer_cpp(
	TARGET_NAME igasset_schema
	SCHEMA_FILE "schema/igasset.fbs"
    BIN_PATH_NAME "igasset/schema")

add_library(igasset ${igasset_headers} ${igasset_sources})
set_property(TARGET igasset PROPERTY CXX_STANDARD 20)
target_include_directories(igasset PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(igasset PUBLIC
	"${CMAKE_BINARY_DIR}"
	"${draco_SOURCE_DIR}/src")
target_link_libraries(igasset PUBLIC draco_lib igasset_schema glm ozz_animation ozz_geometry)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME igasset)
endif()
