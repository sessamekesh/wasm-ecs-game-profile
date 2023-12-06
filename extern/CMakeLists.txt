include(FetchContent)
  
FetchContent_Declare(
    igasync
    GIT_REPOSITORY "https://github.com/sessamekesh/igasync"
    GIT_TAG "a9d971337ffa505fc29650359eb5db13806eb82c"
)
FetchContent_MakeAvailable(igasync)

FetchContent_Declare(
    iggpu
    GIT_REPOSITORY "https://github.com/sessamekesh/iggpu"
    GIT_TAG "61668168fbba106abe7c0257fa18e075bb666c43"
)
FetchContent_MakeAvailable(iggpu)

if (IG_TOOLS_BUILD AND NOT EMSCRIPTEN)
  set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
      assimp
      GIT_REPOSITORY "https://github.com/assimp/assimp"
      GIT_TAG "fa71b84c1"
  )
  FetchContent_MakeAvailable(assimp)
endif ()

set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    draco
    GIT_REPOSITORY "https://github.com/google/draco"
    GIT_TAG "9f856abaafb4b39f1f013763ff061522e0261c6f"
)
FetchContent_MakeAvailable(draco)
if (MSVC)
  add_library(draco_lib ALIAS draco)
else ()
  add_library(draco_lib ALIAS draco_static)
endif ()

# Flatbuffers (used for serializing binary data - assets, network requests, etc)
if (EMSCRIPTEN)
  set(FLATBUFFERS_BUILD_FLATC OFF CACHE BOOL "" FORCE)
endif ()
FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY "https://github.com/google/flatbuffers"
    GIT_TAG "205285c35c164c0368c913b3f611cf8d672bc103"
)
FetchContent_MakeAvailable(flatbuffers)

# Digraph library, used for ECS scheduling verification
FetchContent_Declare(
    digraphlib
    GIT_REPOSITORY "https://github.com/grame-cncm/digraph"
    GIT_TAG "546d79637a904af17e2502d548ff403699c58aa6"
)
FetchContent_GetProperties(digraphlib)
if (NOT digraphlib_POPULATED)
  FetchContent_Populate(digraphlib)
endif ()
add_library(dglib INTERFACE)
target_include_directories(dglib INTERFACE ${digraphlib_SOURCE_DIR})

# EnTT (Entity Component System library that powers core game logic)
if (MSVC)
  set(ENTT_INCLUDE_NATVIS OFF CACHE BOOL "" FORCE)
endif ()
FetchContent_Declare(
    entt
    GIT_REPOSITORY "https://github.com/skypjack/entt"
    GIT_TAG "344e03ac64a1f78424ab1150e2d4778e8df8431d"
)
FetchContent_MakeAvailable(entt)

# CLI11 - used for tools builds
if (IG_TOOLS_BUILD AND NOT EMSCRIPTEN)
  FetchContent_Declare(
    cli11
    GIT_REPOSITORY "https://github.com/CLIUtils/CLI11"
    GIT_TAG "826415f8ba78d18417a6c6deace90a3331b90f79"
  )
  FetchContent_MakeAvailable(cli11)
endif ()

# nlohmann/json - JSON serialization / parser library
FetchContent_Declare(
  nlohmannjson
  GIT_REPOSITORY "https://github.com/nlohmann/json"
  GIT_TAG "6eab7a2b187b10b2494e39c1961750bfd1bda500"
)
FetchContent_MakeAvailable(nlohmannjson)

# ozz-animation (skeletal animation library, WASM friendly)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ozz_build_howtos OFF CACHE BOOL "" FORCE)
set(ozz_build_samples OFF CACHE BOOL "" FORCE)
set(ozz_build_tests OFF CACHE BOOL "" FORCE)
set(ozz_build_fbx OFF CACHE BOOL "" FORCE)
set(ozz_build_gltf OFF CACHE BOOL "" FORCE)
set(ozz_build_tools OFF CACHE BOOL "" FORCE)
set(ozz_build_msvc_rt_dll ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    ozz-animation
    GIT_REPOSITORY "https://github.com/guillaumeblanc/ozz-animation"
    GIT_TAG "36d24203ea3c20e6808e20f726c336c47846b645"
)
FetchContent_MakeAvailable(ozz-animation)

# STB image library (https://github.com/nothings/stb)
# Some adjustments have been made to make it a bit more friendly to use
add_library(stb-image "stb/include/stb/stb_image.h" "stb/stb_image_impl.cc")
set_target_properties(stb-image PROPERTIES CXX_STANDARD 20)
target_include_directories(stb-image PUBLIC stb/include)

# Half-float implementation, useful for HDR things
FetchContent_Declare(
    f16tlib
    GIT_REPOSITORY "https://github.com/fengwang/float16_t"
    GIT_TAG "57164cb59aa57c13110966319150101d3ff12281"
)
FetchContent_GetProperties(f16tlib)
if (NOT f16tlib_POPULATED)
  FetchContent_Populate(f16tlib)
endif ()
add_library(float16t INTERFACE)
target_include_directories(float16t INTERFACE ${f16tlib_SOURCE_DIR})

# Emscripten setup (make sure to build with correct compiler flags for final output binary)
if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME stb-image)
  set_wasm_target_properties(TARGET_NAME igasync)
  set_wasm_target_properties(TARGET_NAME iggpu)
  set_wasm_target_properties(TARGET_NAME flatbuffers)
  set_wasm_target_properties(TARGET_NAME draco_static)
  set_wasm_target_properties(TARGET_NAME ozz_base)
  set_wasm_target_properties(TARGET_NAME ozz_animation)
  set_wasm_target_properties(TARGET_NAME ozz_geometry)
endif ()
