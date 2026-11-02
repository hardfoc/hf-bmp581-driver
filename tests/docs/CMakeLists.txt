# The code shown on the documentation site (docs/ includes these files with pymdownx.snippets), built so a page
# cannot show code that does not compile against the current headers. Compiled, never linked or run.
add_library(hf_bmp581_doc_snippets OBJECT
  quickstart.cpp
  api_tasks.cpp
  configuration.cpp
  ports.cpp
)
target_link_libraries(hf_bmp581_doc_snippets PRIVATE hf::bmp581)
target_include_directories(hf_bmp581_doc_snippets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(hf_bmp581_doc_snippets PRIVATE cxx_std_17)
target_compile_options(hf_bmp581_doc_snippets PRIVATE -Wall -Wextra -Wpedantic)
