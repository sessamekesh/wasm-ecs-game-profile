add_executable(enumerate-assimp main.cc)
target_link_libraries(enumerate-assimp PRIVATE assimp)
set_target_properties(enumerate-assimp PROPERTIES CXX_STANDARD 20)
