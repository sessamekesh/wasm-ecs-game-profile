function (set_wasm_target_properties)
  set(options)
  set(oneValueArgs TARGET_NAME EXPORT_NAME)
  set(multiValueArgs)
  cmake_parse_arguments(AWT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (${AWT_TARGET_NAME} STREQUAL "")
    message(WARNING "WASM target declared but not given a name - skipping")
    return()
  endif ()

  if (NOT EMSCRIPTEN)
    message(WARNING "WASM target ${AWT_TARGET_NAME} declared on a non-WASM build - skipping")
    return()
  endif ()

  set_target_properties(
      ${AWT_TARGET_NAME}
      PROPERTIES
      SUFFIX ".js")

  target_link_options(
    ${AWT_TARGET_NAME} PRIVATE
      "SHELL:--bind -s NO_DYNAMIC_EXECUTION=1 -s WASM=1 -s MODULARIZE=1 -s ENVIRONMENT=web,worker -s WASM_BIGINT=1 -s USE_GLFW=3 -s FETCH=1 -s INITIAL_MEMORY=268435456 -sALLOW_MEMORY_GROWTH -sEXIT_RUNTIME=0 -s MALLOC=emmalloc -s FILESYSTEM=0 -s USE_WEBGPU=1 -lwebsocket.js --no-entry")

  if (NOT ${AWT_EXPORT_NAME} STREQUAL "")
    target_link_options(
      ${AWT_TARGET_NAME} PRIVATE
      "SHELL:-s EXPORT_NAME=${AWT_EXPORT_NAME}")
  endif ()

  target_compile_options(${AWT_TARGET_NAME} PUBLIC -msimd128 -msse2)

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${AWT_TARGET_NAME} PUBLIC -gsource-map -O0)
    target_link_options(${AWT_TARGET_NAME} PRIVATE "SHELL:-gsource-map -O0")
  else ()
    target_compile_options(${AWT_TARGET_NAME} PUBLIC -O3)
    target_link_options(${AWT_TARGET_NAME} PRIVATE "SHELL:-O3")
  endif ()

  if (IG_ENABLE_WASM_THREADS)
    target_compile_options(${AWT_TARGET_NAME} PRIVATE "-pthread")
    target_link_options(
      ${AWT_TARGET_NAME} PRIVATE
        "SHELL:-s ASSERTIONS=1 -s USE_PTHREADS=1 -pthread -sPTHREAD_POOL_SIZE=6")
  endif ()

endfunction ()