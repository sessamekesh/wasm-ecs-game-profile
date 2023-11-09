function (gen_flatbuffer_cpp)
  set(options)
  set(oneValueArgs TARGET_NAME SCHEMA_FILE BIN_PATH_NAME)
  set(multiValueArgs)
  cmake_parse_arguments(GFC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT TARGET flatc)
    message(FATAL_ERROR "flatc binary not available - this is probably because a tools build was not done on WASM")
  endif ()

  get_filename_component(generated_file_name ${GFC_SCHEMA_FILE} NAME_WE)

  set(proto_include_dir "${PROJECT_BINARY_DIR}/flatbuffer/include")
  set(proto_file_out_dir "${proto_include_dir}/${GFC_BIN_PATH_NAME}")
  set(proto_file_out_name "${proto_file_out_dir}/${generated_file_name}.h")
  get_filename_component(protopath_absolute "${GFC_SCHEMA_FILE}" ABSOLUTE)
  set(flatc_args "--no-cpp-direct-copy --filename-suffix \"\" -o ${proto_file_out_dir}")

  file(MAKE_DIRECTORY ${proto_file_out_dir})

  add_custom_command(
      OUTPUT ${proto_file_out_name}
      COMMAND flatc ${flatc_args} --cpp ${protopath_absolute}
      DEPENDS flatc ${protopath_absolute}
  )

  add_library(${GFC_TARGET_NAME} INTERFACE ${proto_file_out_name})
  target_include_directories(${GFC_TARGET_NAME} INTERFACE ${proto_include_dir})
  target_link_libraries(${GFC_TARGET_NAME} INTERFACE flatbuffers)
  set_target_properties(${GFC_TARGET_NAME} PROPERTIES FOLDER flatbuffer_libs)
endfunction ()
