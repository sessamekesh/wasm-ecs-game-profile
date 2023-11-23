# Executes an *igpack-plan to build a collection of asset packs (*.igpack files)
#
# Usage:
# build_ig_asset_pack_plan(
#   TARGET_NAME foo_asset
#   PLAN resources/foo_asset.igpack-plan
#   INDIR ${PROJECT_SOURCE_DIR}
#   OUTDIR ${PROJECT_BINARY_DIR}
#   INFILES
#     foo_image.png
#     foo_geometry.fbx
#   TARGET_OUTPUT_FILES
#     generated/foo_low_lod.igpack
#     generated/foo_medium_lod.igpack
#     generated/foo_high_lod.igpack
#
# Target: igpack-gen (see tools/igpack-gen)
# Input: Execution plan file
# Input: Working directory of the tool (in some output asset directory, usually)
# Input: Sentinel asset file(s) (used to indicate if the tool has run yet)
function (build_ig_asset_pack_plan)
  set (options)
  set(oneValueArgs PLAN TARGET_NAME INDIR OUTDIR PREBUILT_PACK)
  set(multiValueArgs TARGET_OUTPUT_FILES INFILES)
  # hehe, bigpp
  cmake_parse_arguments(BIGPP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  get_filename_component(plan_abs "${BIGPP_PLAN}" ABSOLUTE)

  if (BIGPP_INDIR)
    get_filename_component(indir_abs "${BIGPP_INDIR}" ABSOLUTE)
    set(tool_input_directory ${indir_abs})
  else ()
    set(tool_input_directory ${CMAKE_CURRENT_SOURCE_DIR})
  endif ()

  if (BIGPP_OUTDIR)
    set(tool_output_directory ${BIGPP_OUTDIR})
  else ()
    set(tool_output_directory ${CMAKE_CURRENT_BINARY_DIR})
  endif ()

  foreach (TARGET_OUTFILE IN LISTS BIGPP_TARGET_OUTPUT_FILES)
    get_filename_component(out_abs "${tool_output_directory}/${TARGET_OUTFILE}" ABSOLUTE)
    list(APPEND target_outputs "${out_abs}")
  endforeach ()

  foreach (TARGET_INFILE IN LISTS BIGPP_INFILES)
    get_filename_component(in_abs "${tool_input_directory}/${TARGET_INFILE}" ABSOLUTE)
    list(APPEND target_inputs "${in_abs}")
  endforeach ()

  if (IG_USE_PREBUILT_ASSETS AND BIGPP_PREBUILT_PACK AND EXISTS "${tool_input_directory}/${BIGPP_PREBUILT_PACK}")
    get_filename_component(prebuild_abs "${tool_input_directory}/${BIGPP_PREBUILT_PACK}" ABSOLUTE)
    add_custom_command(
      OUTPUT ${target_outputs}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${tool_output_directory}
      COMMAND ${CMAKE_COMMAND} -E copy ${prebuild_abs} ${target_outputs}
    )
    add_custom_target(${BIGPP_TARGET_NAME} SOURCES ${target_outputs})
    set_target_properties(${BIGPP_TARGET_NAME} PROPERTIES FOLDER assets)
    return ()
  endif ()

  if (NOT TARGET igpack-gen)
    message(FATAL_ERROR "igpack-gen binary not available - this is usually because a tools build was not finished on WASM")
  endif ()

  add_custom_command(
    OUTPUT ${target_outputs}
    COMMAND igpack-gen --input_asset_path_root=${tool_input_directory}
                       --output_asset_path_root=${tool_output_directory}
                       --input_plan_file=${plan_abs}
                       --schema=${CMAKE_SOURCE_DIR}/tools/igpack-gen/schema/igpack-plan.fbs
    DEPENDS ${plan_abs} igpack-gen ${target_inputs}
            ${CMAKE_SOURCE_DIR}/tools/igpack-gen/schema/igpack-plan.fbs
    VERBATIM)

  add_custom_target(${BIGPP_TARGET_NAME} SOURCES ${target_outputs})
  set_target_properties(${BIGPP_TARGET_NAME} PROPERTIES FOLDER assets)
endfunction ()
