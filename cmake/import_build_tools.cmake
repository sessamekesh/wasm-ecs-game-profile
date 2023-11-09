#
# Import build tools
#
# If pre-built native build tools are available, import them.
# Required for emscripten builds!
#
if (EMSCRIPTEN)
  if (NOT IG_TOOL_WRANGLE_PATH)
    message(FATAL_ERROR "Must provide tool wrangling path with -DIG_TOOL_WRANGLE_PATH=\"some_path\"")
  endif ()
  include(${IG_TOOL_WRANGLE_PATH} RESULT_VARIABLE tool_wrangle_rsl)
  if (tool_wrangle_rsl EQUAL NOTFOUND)
	message(FATAL_ERROR "Tool wrangling failed! Could not find tool wrangler script at ${IG_TOOL_WRANGLE_PATH}")
  endif()

  message(STATUS "Tool wrangling succeeded! Tools have been read from path ${IG_TOOL_WRANGLE_PATH}")

  get_property(flatc_location TARGET flatc PROPERTY LOCATION)
  get_property(igpack_gen_location TARGET igpack-gen PROPERTY LOCATION)
  message(STATUS "--- flatc location: ${flatc_location}")
  message(STATUS "--- igpack-gen location: ${igpack_gen_location}")
endif ()
