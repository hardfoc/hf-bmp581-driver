# hf_bmp581 — ESP-IDF component wrapper (header-only driver)
set(HF_BMP581_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." CACHE PATH
    "Path to hf-bmp581-driver root")

get_filename_component(HF_BMP581_ROOT "${HF_BMP581_ROOT}" ABSOLUTE)
if(NOT EXISTS "${HF_BMP581_ROOT}/cmake/hf_bmp581_build_settings.cmake")
    message(FATAL_ERROR
        "Cannot find HF-BMP581 driver root at ${HF_BMP581_ROOT}")
endif()

include("${HF_BMP581_ROOT}/cmake/hf_bmp581_build_settings.cmake")

idf_component_register(
    INCLUDE_DIRS ${HF_BMP581_PUBLIC_INCLUDE_DIRS}
    REQUIRES ${HF_BMP581_IDF_REQUIRES}
)

target_compile_features(${COMPONENT_LIB} INTERFACE cxx_std_17)
