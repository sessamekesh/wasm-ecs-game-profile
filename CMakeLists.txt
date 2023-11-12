cmake_minimum_required(VERSION 3.26)

project(
  wasm-ecs-game-profile VERSION 1.0
  DESCRIPTION "Excerpt of game code with profiling attached to test performance on native and WASM builds"
  LANGUAGES C CXX)

set(IG_TOOL_WRANGLE_PATH "${CMAKE_BINARY_DIR}/wrangle-tools.cmake" CACHE FILEPATH "Location of the native tool wrangling CMake file - native builds write to this path, WASM builds read from it")
set(IG_TOOLS_BUILD "ON" CACHE BOOL "Build tools if this option is set to true")
set(IG_ENABLE_ECS_VALIDATION "ON" CACHE BOOL "Include asserts that validate ECS concurrency safety (useful in debugging, but creates loud errors)")

include(cmake/import_build_tools.cmake)
include(cmake/gen_flatbuffer_cpp.cmake)
include(cmake/wasm_helpers.cmake)

add_subdirectory(extern)
add_subdirectory(lib)

if (IG_TOOLS_BUILD AND NOT EMSCRIPTEN)
  message(STATUS "Tooling added for this project - don't forget to export them if you're doing a web build")
  add_subdirectory(tools)
endif ()

include(cmake/export_build_tools.cmake)
include(cmake/gen_igpack.cmake)

#
# Project itself
#
set(igdemo_includes
  "include/igdemo/assets/core-shaders.h"
  "include/igdemo/assets/ybot.h"
  "include/igdemo/logic/framecommon.h"
  "include/igdemo/render/animated-pbr.h"
  "include/igdemo/render/ctx-components.h"
  "include/igdemo/render/pbr-geo-pass.h"
  "include/igdemo/render/tonemap-pass.h"
  "include/igdemo/render/skeletal-animation.h"
  "include/igdemo/render/wgpu-helpers.h"
  "include/igdemo/render/world-transform-component.h"
  "include/igdemo/systems/animation.h"
  "include/igdemo/igdemo-app.h"
  "include/igdemo/scheduler.h")

set(igdemo_sources
  "igdemo/assets/core-shaders.cc"
  "igdemo/assets/ybot.cc"
  "igdemo/render/animated-pbr.cc"
  "igdemo/render/ctx-components.cc"
  "igdemo/render/pbr-geo-pass.cc"
  "igdemo/render/tonemap-pass.cc"
  "igdemo/render/wgpu-helpers.cc"
  "igdemo/systems/animation.cc"
  "igdemo/igdemo-app.cc"
  "igdemo/scheduler.cc")

if (EMSCRIPTEN)
  set(igdemo_platform_sources
    "include/igdemo/platform/web/raii-fetch.h"
    "igdemo/platform/web/raii-fetch.cc"
    "igdemo/wasm-bindings.cc")
else ()
  set(igdemo_platform_sources
    "igdemo/native-main.cc")
endif ()

add_executable(igdemo ${igdemo_includes} ${igdemo_sources} ${igdemo_platform_sources})
target_include_directories(igdemo PRIVATE "include")
target_link_libraries(igdemo PUBLIC iggpu igasync igecs igasset)
set_property(TARGET igdemo PROPERTY CXX_STANDARD 20)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME igdemo EXPORT_NAME IgDemoModule)

  configure_file(webview/index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)
else ()
  target_link_libraries(igdemo PUBLIC CLI11)
endif ()

BUILD_IG_ASSET_PACK_PLAN(
    TARGET_NAME igp-ybot
    PLAN assets/ybot.igpack-plan.json
    INDIR assets
    INFILES
      "defeated.fbx"
      "idle.fbx"
      "run.fbx"
      "walk.fbx"
      "ybot-geo.fbx"
    TARGET_OUTPUT_FILES
      "resources/ybot.igpack")

BUILD_IG_ASSET_PACK_PLAN(
    TARGET_NAME igp-shaders
    PLAN assets/shaders.igpack-plan.json
    INDIR assets
    INFILES
      "shaders/aces_tonemapping.wgsl"
      "shaders/animated_pbr.wgsl"
    TARGET_OUTPUT_FILES
      "resources/shaders.igpack")

add_dependencies(igdemo
  igp-ybot igp-shaders)
