# Whole-program Tekken 3 product. The authenticated executable remains runtime data; psxport's
# Lightrec executor owns every non-native guest instruction.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/tekken3/executable.json" TEKKEN3_EXECUTABLE_MANIFEST)
string(JSON TEKKEN3_EXECUTABLE_SHA256 GET "${TEKKEN3_EXECUTABLE_MANIFEST}" sha256)
string(JSON TEKKEN3_EXECUTABLE_SIZE GET "${TEKKEN3_EXECUTABLE_MANIFEST}" file_size)
string(JSON TEKKEN3_DISC_EXECUTABLE GET "${TEKKEN3_EXECUTABLE_MANIFEST}" disc_executable)
string(JSON TEKKEN3_EXECUTABLE_ENTRY GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header entry)
string(JSON TEKKEN3_EXECUTABLE_GP GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header gp)
string(JSON TEKKEN3_EXECUTABLE_TEXT GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header text_address)
string(JSON TEKKEN3_EXECUTABLE_TEXT_SIZE GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header text_size)
string(JSON TEKKEN3_EXECUTABLE_STACK GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header stack_address)
string(JSON TEKKEN3_EXECUTABLE_STACK_OFFSET GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header stack_offset)
configure_file(game/entry/executable_identity.h.in title_metadata/executable_identity.h @ONLY)

psxport_add_product(
  tekken3_port
  CATALOG tekken3::createCatalog
  HEADER entry/tekken3_catalog.h
  SOURCES game/entry/tekken3_catalog.cpp
  LIBRARIES tekken3_runtime
)
add_dependencies(tekken3_port gen_gpu_shaders)
target_compile_features(tekken3_port PRIVATE cxx_std_20)
target_include_directories(tekken3_port PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
set_target_properties(
  tekken3_port PROPERTIES
  ENABLE_EXPORTS ON
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME tekken3_product_help_contract COMMAND tekken3_port --help)
set_tests_properties(tekken3_product_help_contract PROPERTIES
  PASS_REGULAR_EXPRESSION "Usage: tekken3_port \\[executable\\]")
