cmake_minimum_required(VERSION 3.12)
project(hesiod-root VERSION 0.5.2)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Version info
add_compile_definitions(
  HESIOD_VERSION_MAJOR=${hesiod-root_VERSION_MAJOR}
  HESIOD_VERSION_MINOR=${hesiod-root_VERSION_MINOR}
  HESIOD_VERSION_PATCH=${hesiod-root_VERSION_PATCH})

# ------------------------------
# Options
# ------------------------------
set(HESIOD_ENABLE_GENERATE_APP_IMAGE_DEFAULT ON)
if(APPLE)
  set(HESIOD_ENABLE_GENERATE_APP_IMAGE_DEFAULT OFF)
endif()

option(HESIOD_ENABLE_GENERATE_APP_IMAGE "Enable AppImage generation"
       ${HESIOD_ENABLE_GENERATE_APP_IMAGE_DEFAULT})

option(HESIOD_MINIMAL_NODE_SET
       "Use a minimal set of nodes (ADVANCED DEV ONLY!!!)" OFF)

# Option to enable LTO and unused-function detection
option(HESIOD_ENABLE_LTO "Enable LTO and unused-function detection" OFF)
option(HESIOD_UNUSED_FUNCTIONS "Detect unused static functions" OFF)

# ------------------------------
# Modules
# ------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(HesiodCompilerOptions)
include(HesiodPlatform)
include(HesiodQtLoggingRules)
include(HesiodGitVersion)

# ------------------------------
# Dependencies
# ------------------------------
find_package(spdlog REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets
                                     WebEngineWidgets)
find_package(nlohmann_json REQUIRED)

# Apple's OpenCL framework does not provide the C++ headers used by CLWrapper.
# On macOS, FindOpenCL pairs the framework with the separately installed header
# package and this include path is inherited by the nested HighMap build.
if(APPLE)
  find_package(OpenCL REQUIRED)
  find_path(HESIOD_OPENCL_CPP_INCLUDE_DIR CL/opencl.hpp)
  if(NOT HESIOD_OPENCL_CPP_INCLUDE_DIR)
    message(FATAL_ERROR
            "macOS requires the OpenCL C++ headers (CL/opencl.hpp) to build CLWrapper.")
  endif()
  include_directories(SYSTEM ${OpenCL_INCLUDE_DIR}
                             ${HESIOD_OPENCL_CPP_INCLUDE_DIR})
endif()

add_compile_definitions(
  CL_HPP_MINIMUM_OPENCL_VERSION=120 CL_HPP_TARGET_OPENCL_VERSION=120
  HSD_DEFAULT_GPU_MODE=true)

# ------------------------------
# Subdirectories
# ------------------------------
add_subdirectory(external)
add_subdirectory(Hesiod)

if(HESIOD_ENABLE_GENERATE_APP_IMAGE)
  add_subdirectory(scripts/generate_appimage)
endif()
