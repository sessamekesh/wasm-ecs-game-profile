cmake_minimum_required(VERSION 3.26)

project(
  wasm-ecs-game-profile VERSION 1.0
  DESCRIPTION "Excerpt of game code with profiling attached to test performance on native and WASM builds"
  LANGUAGES C CXX)

set(IG_TOOL_WRANGLE_PATH "${CMAKE_BINARY_DIR}/wrangle-tools.cmake" CACHE FILEPATH "Location of the native tool wrangling CMake file - native builds write to this path, WASM builds read from it")
set(IG_TOOLS_BUILD "ON" CACHE BOOL "Build tools if this option is set to true")
set(IG_ENABLE_ECS_VALIDATION "ON" CACHE BOOL "Include asserts that validate ECS concurrency safety (useful in debugging, but creates loud errors)")
set(IG_ENABLE_WASM_THREADS "ON" CACHE BOOL "Build targets with threading support")
set(IG_USE_PREBUILT_ASSETS "ON" CACHE BOOL "Use pre-generated asset packs - useful since not all targets can be built from distributed source")
set(IG_BUILD_TESTS "OFF" CACHE BOOL "Use pre-generated asset packs - useful since not all targets can be built from distributed source")

include(cmake/import_build_tools.cmake)
include(cmake/gen_flatbuffer_cpp.cmake)
include(cmake/wasm_helpers.cmake)

add_subdirectory(extern)
add_subdirectory(lib)

if (IG_TOOLS_BUILD AND NOT EMSCRIPTEN)
  message(STATUS "Tooling added for this project - don't forget to export them if you're doing a web build")
  add_subdirectory(tools)
endif ()

if (IG_BUILD_TESTS)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY "https://github.com/google/googletest"
    GIT_TAG "6b74da4757a549563d7c37c8fae3e704662a043b"
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  include(GoogleTest)
endif ()

include(cmake/export_build_tools.cmake)
include(cmake/gen_igpack.cmake)

#
# Project itself
#
set(igdemo_includes
  "include/igdemo/assets/core-shaders.h"
  "include/igdemo/assets/projectiles.h"
  "include/igdemo/assets/skybox.h"
  "include/igdemo/assets/ybot.h"
  "include/igdemo/logic/combat.h"
  "include/igdemo/logic/enemy-strategy.h"
  "include/igdemo/logic/enemy.h"
  "include/igdemo/logic/framecommon.h"
  "include/igdemo/logic/hero.h"
  "include/igdemo/logic/levelmetadata.h"
  "include/igdemo/logic/locomotion.h"
  "include/igdemo/logic/projectile.h"
  "include/igdemo/logic/renderable.h"
  "include/igdemo/logic/spatial-index.h"
  "include/igdemo/platform/input-emitter.h"
  "include/igdemo/platform/keyboard-mouse-input-emitter.h"
  "include/igdemo/render/geo/cube.h"
  "include/igdemo/render/geo/fullscreen_quad.h"
  "include/igdemo/render/geo/sphere.h"
  "include/igdemo/render/processing/brdflut.h"
  "include/igdemo/render/processing/equirect-to-cubemap.h"
  "include/igdemo/render/processing/gen-mips.h"
  "include/igdemo/render/processing/irradiance-map-generator.h"
  "include/igdemo/render/processing/prefilter-env-gen.h"
  "include/igdemo/render/animated-pbr.h"
  "include/igdemo/render/bg-skybox.h"
  "include/igdemo/render/camera.h"
  "include/igdemo/render/ctx-components.h"
  "include/igdemo/render/pbr-common.h"
  "include/igdemo/render/skeletal-animation.h"
  "include/igdemo/render/static-pbr.h"
  "include/igdemo/render/wgpu-helpers.h"
  "include/igdemo/render/world-transform-component.h"
  "include/igdemo/systems/animation.h"
  "include/igdemo/systems/attach-renderables.h"
  "include/igdemo/systems/destroy-actor.h"
  "include/igdemo/systems/enemy-locomotion.h"
  "include/igdemo/systems/fly-camera.h"
  "include/igdemo/systems/hero-locomotion.h"
  "include/igdemo/systems/locomotion.h"
  "include/igdemo/systems/move-projectile.h"
  "include/igdemo/systems/pbr-geo-pass.h"
  "include/igdemo/systems/projectile-hit.h"
  "include/igdemo/systems/skybox.h"
  "include/igdemo/systems/spawn-projectiles.h"
  "include/igdemo/systems/tonemap-pass.h"
  "include/igdemo/systems/update-health.h"
  "include/igdemo/systems/update-spatial-index.h"
  "include/igdemo/igdemo-app.h"
  "include/igdemo/scheduler.h")

set(igdemo_sources
  "igdemo/assets/core-shaders.cc"
  "igdemo/assets/projectiles.cc"
  "igdemo/assets/skybox.cc"
  "igdemo/assets/ybot.cc"
  "igdemo/logic/enemy.cc"
  "igdemo/logic/hero.cc"
  "igdemo/logic/spatial-index.cc"
  "igdemo/platform/keyboard-mouse-input-emitter.cc"
  "igdemo/render/geo/cube.cc"
  "igdemo/render/geo/fullscreen_quad.cc"
  "igdemo/render/geo/sphere.cc"
  "igdemo/render/processing/brdflut.cc"
  "igdemo/render/processing/equirect-to-cubemap.cc"
  "igdemo/render/processing/gen-mips.cc"
  "igdemo/render/processing/irradiance-map-generator.cc"
  "igdemo/render/processing/prefilter-env-gen.cc"
  "igdemo/render/animated-pbr.cc"
  "igdemo/render/bg-skybox.cc"
  "igdemo/render/ctx-components.cc"
  "igdemo/render/static-pbr.cc"
  "igdemo/render/wgpu-helpers.cc"
  "igdemo/systems/animation.cc"
  "igdemo/systems/attach-renderables.cc"
  "igdemo/systems/destroy-actor.cc"
  "igdemo/systems/enemy-locomotion.cc"
  "igdemo/systems/fly-camera.cc"
  "igdemo/systems/hero-locomotion.cc"
  "igdemo/systems/locomotion.cc"
  "igdemo/systems/move-projectile.cc"
  "igdemo/systems/pbr-geo-pass.cc"
  "igdemo/systems/projectile-hit.cc"
  "igdemo/systems/skybox.cc"
  "igdemo/systems/spawn-projectiles.cc"
  "igdemo/systems/tonemap-pass.cc"
  "igdemo/systems/update-health.cc"
  "igdemo/systems/update-spatial-index.cc"
  "igdemo/igdemo-app.cc"
  "igdemo/scheduler.cc")

if (EMSCRIPTEN)
  set(igdemo_platform_sources
    "include/igdemo/platform/web/raii-fetch.h"
    "igdemo/platform/web/raii-fetch.cc")
  set(igdemo_main "igdemo/wasm-bindings.cc")
else ()
  set(igdemo_platform_sources)
  set(igdemo_main "igdemo/native-main.cc")
endif ()

add_library(igdemo_lib ${igdemo_includes} ${igdemo_sources} ${igdemo_platform_sources})
target_include_directories(igdemo_lib PUBLIC "include")
target_link_libraries(igdemo_lib PUBLIC iggpu igasync igecs igasset)
set_property(TARGET igdemo_lib PROPERTY CXX_STANDARD 20)

add_executable(igdemo ${igdemo_main})
target_link_libraries(igdemo PRIVATE igdemo_lib)
set_property(TARGET igdemo PROPERTY CXX_STANDARD 20)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME igdemo_lib)
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
    PREBUILT_PACK
      "prebuilt/ybot.igpack"
    TARGET_OUTPUT_FILES
      "resources/ybot.igpack")

BUILD_IG_ASSET_PACK_PLAN(
    TARGET_NAME igp-shaders
    PLAN assets/shaders.igpack-plan.json
    INDIR assets
    INFILES
      "shaders/aces_tonemapping.wgsl"
      "shaders/animated_pbr.wgsl"
      "shaders/static_pbr.wgsl"
    TARGET_OUTPUT_FILES
      "resources/shaders.igpack")

BUILD_IG_ASSET_PACK_PLAN(
    TARGET_NAME igp-skybox
    PLAN assets/skybox.igpack-plan.json
    INDIR assets
    INFILES
      "shaders/bg_skybox.wgsl"
      "shaders/brdf_lut.wgsl"
      "shaders/hdr_mipgen.wgsl"
      "shaders/equirect_to_cubemap.wgsl"
      "shaders/irradiance_gen.wgsl"
      "shaders/prefilter_gen.wgsl"
      "industrial_sunset_02_puresky_2k.hdr"
    TARGET_OUTPUT_FILES
      "resources/skybox.igpack")

add_dependencies(igdemo
  igp-ybot igp-shaders igp-skybox)


if (IG_BUILD_TESTS)
  set(igdemo_test_sources
    "test/entt-usage-test.cc")
  add_executable(igdemo_tests ${igdemo_test_sources})
  target_link_libraries(igdemo_tests PUBLIC igdemo_lib gtest gtest_main)
  set_property(TARGET igdemo_tests PROPERTY CXX_STANDARD 20)

  if (NOT EMSCRIPTEN)
    gtest_discover_tests(igdemo_tests)
  endif ()
endif ()
