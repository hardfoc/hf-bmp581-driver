# Host unit tests — no hardware; fake I2C adapter replays data-sheet frames.
add_executable(hf_bmp581_host_tests bmp581_driver_test.cpp)
target_link_libraries(hf_bmp581_host_tests PRIVATE hf::bmp581)
target_compile_options(hf_bmp581_host_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME hf_bmp581_host_tests COMMAND hf_bmp581_host_tests)
