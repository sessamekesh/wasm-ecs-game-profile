#
# Export build tools
#
# Run in a native build to assemble a CMake file that includes
#  binary targets that can't be built in Emscripten builds
# (e.g. asset pipeline tools)
#
if (NOT EMSCRIPTEN AND IG_TOOL_WRANGLE_PATH)
  message(STATUS "Writing import-igtools.cmake file to ${IG_TOOL_WRANGLE_PATH}")
  export(TARGETS flatc igpack-gen FILE "${IG_TOOL_WRANGLE_PATH}")
endif()
