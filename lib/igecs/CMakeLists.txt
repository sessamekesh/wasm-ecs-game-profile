configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/igecs/config.h"
)

set(igecs_headers
  "include/igecs/profile/frame_profiler.h"
  "include/igecs/ctti_type_id.h"
  "include/igecs/evt_queue.h"
  "include/igecs/scheduler.h"
  "include/igecs/world_view.h")

set(igecs_sources
  "src/profile/frame_profiler.cc"
  "src/ctti_type_id.cc"
  "src/scheduler.cc"
  "src/world_view.cc")

set(igecs_test_sources
  "test/ctti_type_id_test.cc"
  "test/scheduler_test.cc"
  "test/world_view_test.cc")

add_library(igecs ${igecs_headers} ${igecs_sources})

target_include_directories(igecs PUBLIC include "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(igecs PUBLIC igasync EnTT nlohmann_json)
target_link_libraries(igecs PRIVATE dglib)

set_property(TARGET igecs PROPERTY CXX_STANDARD 20)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME igecs)
endif()

if (IG_BUILD_TESTS)
  add_executable(igecs-test ${igecs_test_sources})
  target_link_libraries(igecs-test PUBLIC gtest gtest_main igecs)

  if (NOT EMSCRIPTEN)
	gtest_discover_tests(igecs-test
	  WORKING_DIRECTORY ${PROJECT_DIR}
	  PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}")
  endif ()

  set_target_properties(igecs-test PROPERTIES FOLDER tests)
  set_property(TARGET igecs-test PROPERTY CXX_STANDARD 20)
endif ()
