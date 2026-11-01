#===============================================================================
# HF-BMP581 Driver - Build settings
#===============================================================================

include_guard(GLOBAL)

set(HF_BMP581_TARGET_NAME "hf_bmp581")

set(HF_BMP581_VERSION_MAJOR 1)
set(HF_BMP581_VERSION_MINOR 0)
set(HF_BMP581_VERSION_PATCH 0)
set(HF_BMP581_VERSION
    "${HF_BMP581_VERSION_MAJOR}.${HF_BMP581_VERSION_MINOR}.${HF_BMP581_VERSION_PATCH}")
set(HF_BMP581_VERSION_STRING "${HF_BMP581_VERSION}")

set(HF_BMP581_VERSION_TEMPLATE
    "${CMAKE_CURRENT_LIST_DIR}/../inc/bmp581_version.h.in")
set(HF_BMP581_VERSION_HEADER_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/hf_bmp581_generated")
set(HF_BMP581_VERSION_HEADER
    "${HF_BMP581_VERSION_HEADER_DIR}/bmp581_version.h")

file(MAKE_DIRECTORY "${HF_BMP581_VERSION_HEADER_DIR}")

if(EXISTS "${HF_BMP581_VERSION_TEMPLATE}")
    configure_file(
        "${HF_BMP581_VERSION_TEMPLATE}"
        "${HF_BMP581_VERSION_HEADER}"
        @ONLY
    )
    message(STATUS
        "HF-BMP581 driver v${HF_BMP581_VERSION} — generated bmp581_version.h in ${HF_BMP581_VERSION_HEADER_DIR}")
else()
    message(WARNING "bmp581_version.h.in not found at ${HF_BMP581_VERSION_TEMPLATE}")
endif()

set(HF_BMP581_PUBLIC_INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../inc"
    "${HF_BMP581_VERSION_HEADER_DIR}"
)

set(HF_BMP581_SOURCE_FILES "")

set(HF_BMP581_IDF_REQUIRES driver)
