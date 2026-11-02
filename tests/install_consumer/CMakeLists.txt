# Consumer of an installed hf-bmp581-driver: find_package() + the documented target name. Built by the
# hf_bmp581_install_consumer test (tests/install_consumer.cmake) against a fresh `cmake --install`.
cmake_minimum_required(VERSION 3.16)
project(hf_bmp581_install_consumer LANGUAGES CXX)

find_package(hf_bmp581 CONFIG REQUIRED)

add_library(consumer OBJECT main.cpp)
target_link_libraries(consumer PRIVATE hf::bmp581)
target_compile_options(consumer PRIVATE -Wall -Wextra -Wpedantic -Werror)
