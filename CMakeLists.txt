cmake_minimum_required(VERSION 3.16)
project(VectorDrift VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

set(SOURCES
  src/main.cpp
  src/game.cpp
  src/ship.cpp
  src/weapon.cpp
  src/enemy_ai.cpp
  src/renderer.cpp
  src/hud.cpp
  src/audio.cpp
  src/particles.cpp
  src/mesh.cpp
  src/camera.cpp
)

add_executable(vector_drift ${SOURCES})
target_include_directories(vector_drift PRIVATE
  include
  third_party
  third_party/glad/include
  ${glm_SOURCE_DIR}
)
target_link_libraries(vector_drift PRIVATE
  glad
  glfw
  OpenGL::GL
  Threads::Threads
  ${CMAKE_DL_LIBS}
  m
)
target_compile_definitions(vector_drift PRIVATE
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(vector_drift PRIVATE -Wall -Wextra -O2)
endif()

# Copy shaders next to binary
add_custom_command(TARGET vector_drift POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:vector_drift>/shaders
)
