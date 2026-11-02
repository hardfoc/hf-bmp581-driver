# Host unit tests — no hardware; fake I2C adapter replays data-sheet frames.
add_executable(hf_bmp581_host_tests bmp581_driver_test.cpp)
target_link_libraries(hf_bmp581_host_tests PRIVATE hf::bmp581)
target_compile_options(hf_bmp581_host_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME hf_bmp581_host_tests COMMAND hf_bmp581_host_tests)

# `cmake --install` must ship every header the public headers include (inc/*.hpp, the generated bmp581_version.h)
# and export the documented target hf::bmp581. Installs into the build tree and builds a find_package() consumer.
add_test(NAME hf_bmp581_install_consumer
  COMMAND ${CMAKE_COMMAND} -DBIN=${PROJECT_BINARY_DIR} -DSRC=${PROJECT_SOURCE_DIR}
          -DCXX=${CMAKE_CXX_COMPILER} -P ${PROJECT_SOURCE_DIR}/tests/install_consumer.cmake)
