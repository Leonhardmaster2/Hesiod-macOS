add_library(hesiod_platform INTERFACE)

# macOS
if(APPLE)
  message(STATUS "Platform: macOS")
  target_compile_definitions(hesiod_platform INTERFACE HSD_OS_MACOS)

# Linux
elseif(UNIX)
  message(STATUS "Platform: Linux")
  target_compile_definitions(hesiod_platform INTERFACE HSD_OS_LINUX)

  # Windows
elseif(WIN32)
  message(STATUS "Platform: Windows")

  # Unsupported platforms
else()
  message(
    FATAL_ERROR "Unsupported platform. Only macOS, Linux and Windows are supported.")
endif()
