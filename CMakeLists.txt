cmake_minimum_required(VERSION 3.16)
project(gpu_bead LANGUAGES C CXX)


include(FetchContent)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.0
)

FetchContent_MakeAvailable(SDL3)
find_package(OpenGL REQUIRED)

set(GPU_SOURCES
  ${CMAKE_SOURCE_DIR}/main.cpp
  ${CMAKE_SOURCE_DIR}/vendor/glad/src/glad.c
)

add_executable(gpu_bead ${GPU_SOURCES})


target_include_directories(gpu_bead PUBLIC
  ${CMAKE_SOURCE_DIR}/vendor/glad/include
  ${CMAKE_SOURCE_DIR}/vendor
)

target_link_libraries(gpu_bead PUBLIC SDL3::SDL3 OpenGL::GL)

add_custom_target(CopyResources ALL
  COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:gpu_bead>/sources"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/sources"
          "$<TARGET_FILE_DIR:gpu_bead>/sources"
  COMMENT "Copying resources to build folder"
)