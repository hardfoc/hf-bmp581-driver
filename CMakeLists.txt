# =============================================================================
# hf-bmp581-driver — Root CMakeLists.txt
# =============================================================================
# Hardware-agnostic header-only driver for the Bosch BMP581 barometric pressure
# sensor (I2C or SPI via a CRTP register transport; BMP5 API register sequences).
# Integrate via cmake/hf_bmp581_build_settings.cmake or the ESP-IDF component
# under examples/esp32/components/hf_bmp581.
# =============================================================================
cmake_minimum_required(VERSION 3.16)

project(hf_bmp581
  VERSION 1.0.0
  LANGUAGES CXX
)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_bmp581_build_settings.cmake)

include(GNUInstallDirs)

add_library(${HF_BMP581_TARGET_NAME} INTERFACE)
foreach(dir IN LISTS HF_BMP581_PUBLIC_INCLUDE_DIRS)
  target_include_directories(${HF_BMP581_TARGET_NAME} INTERFACE $<BUILD_INTERFACE:${dir}>)
endforeach()
target_include_directories(${HF_BMP581_TARGET_NAME}
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(${HF_BMP581_TARGET_NAME} INTERFACE cxx_std_17)

add_library(hf::bmp581 ALIAS ${HF_BMP581_TARGET_NAME})
# Installed copies export the same name: hf:: + EXPORT_NAME = hf::bmp581.
set_target_properties(${HF_BMP581_TARGET_NAME} PROPERTIES EXPORT_NAME bmp581)

option(HF_BMP581_ENABLE_WARNINGS "Enable compiler warnings for consumers" OFF)
if(HF_BMP581_ENABLE_WARNINGS)
  target_compile_options(${HF_BMP581_TARGET_NAME}
      INTERFACE -Wall -Wextra -Wpedantic)
endif()

option(HF_BMP581_BUILD_TESTS "Build host unit tests (no hardware)" OFF)
if(HF_BMP581_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests/host)
  add_subdirectory(tests/docs)
endif()

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hf_bmp581ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hf_bmp581Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/hf_bmp581Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_bmp581
)

# Install: every header the public headers include (inc/*.hpp and the generated bmp581_version.h), the exported
# target hf::bmp581 and the package files, for find_package(hf_bmp581).
install(TARGETS ${HF_BMP581_TARGET_NAME} EXPORT hf_bmp581Targets)
install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
install(FILES "${HF_BMP581_VERSION_HEADER}" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT hf_bmp581Targets
  FILE hf_bmp581Targets.cmake
  NAMESPACE hf::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_bmp581
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/hf_bmp581Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/hf_bmp581ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_bmp581
)
