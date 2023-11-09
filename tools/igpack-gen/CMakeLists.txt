gen_flatbuffer_cpp(
	TARGET_NAME igpack_plan_schema
	SCHEMA_FILE "schema/igpack-plan.fbs"
    BIN_PATH_NAME "igpack-gen/schema")

set(igpack-gen-headers
  "include/igpack-gen/assimp-animation-processor.h"
  "include/igpack-gen/assimp-geo-processor.h"
  "include/igpack-gen/assimp-util.h"
  "include/igpack-gen/plan-executor.h"
  "include/igpack-gen/wgsl-processor.h")

set(igpack-gen-src
  "src/assimp-animation-processor.cc"
  "src/assimp-geo-processor.cc"
  "src/assimp-util.cc"
  "src/plan-executor.cc"
  "src/wgsl-processor.cc"
  "main.cc")

add_executable(igpack-gen ${igpack-gen-headers} ${igpack-gen-src})
set_target_properties(igpack-gen PROPERTIES CXX_STANDARD 20)
target_link_libraries(igpack-gen
	PRIVATE igasset_schema igpack_plan_schema igasset CLI11 assimp
			draco_lib ozz_animation ozz_animation_offline)
target_include_directories(igpack-gen PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_BINARY_DIR}"
	"${draco_SOURCE_DIR}/src")
