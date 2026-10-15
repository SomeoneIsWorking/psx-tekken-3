cmake_minimum_required(VERSION 3.24)
project(tekken3_port LANGUAGES C CXX)

enable_testing()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Framework checkout: a symlink or clone set up by tools/psxport_fetch.py --auto.
set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: external/psxport)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — run "
                      "`uv run --frozen python tools/psxport_fetch.py --auto`, or point PSXPORT_DIR at a psxport checkout.")
endif()
add_subdirectory("${PSXPORT_DIR}" psxport_build)

# The authenticated executable stays runtime data; its identity is generated from the tracked manifest.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/tekken3/executable.json" TEKKEN3_EXECUTABLE_MANIFEST)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/titles/tekken3/executable.json")
string(JSON TEKKEN3_EXECUTABLE_SHA256 GET "${TEKKEN3_EXECUTABLE_MANIFEST}" sha256)
string(JSON TEKKEN3_EXECUTABLE_SIZE GET "${TEKKEN3_EXECUTABLE_MANIFEST}" file_size)
string(JSON TEKKEN3_DISC_EXECUTABLE GET "${TEKKEN3_EXECUTABLE_MANIFEST}" disc_executable)
string(JSON TEKKEN3_EXECUTABLE_ENTRY GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header entry)
string(JSON TEKKEN3_EXECUTABLE_GP GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header gp)
string(JSON TEKKEN3_EXECUTABLE_TEXT GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header text_address)
string(JSON TEKKEN3_EXECUTABLE_TEXT_SIZE GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header text_size)
string(JSON TEKKEN3_EXECUTABLE_STACK GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header stack_address)
string(JSON TEKKEN3_EXECUTABLE_STACK_OFFSET GET "${TEKKEN3_EXECUTABLE_MANIFEST}" header stack_offset)
configure_file(game/product/executable_identity.h.in title_metadata/executable_identity.h @ONLY)

# The title's owners; the product and every test link them.
add_library(
  tekken3_game STATIC
  game/cd/sector_ready_order.cpp
  game/execution/finite_guest_call.cpp
  game/fieldclock/field_clock.cpp
  game/frame/fight_warp.cpp
  game/frame/finite_frame.cpp
  game/frame/frame_driver.cpp
  game/render/gpu_queue_timeout.cpp
  game/runtime/runtime.cpp
  game/runtime/session.cpp
  game/widescreen/stage_tile_wedge.cpp
  game/widescreen/widescreen.cpp)
target_include_directories(tekken3_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/game")
target_compile_features(tekken3_game PUBLIC cxx_std_20)
target_link_libraries(tekken3_game PUBLIC psxport)

psxport_add_product(
  tekken3_port
  CATALOG tekken3::createCatalog
  HEADER product/catalog.h
  SOURCES game/product/catalog.cpp
  LIBRARIES tekken3_game
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

set(TEKKEN3_EXE "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/tekken3/SLUS_004.02")

foreach(test_name
        bounded_guest_call finite_frame_contract finite_guest_call_contract
        fight_warp_contract frame_driver_contract gpu_queue_timeout_contract mode_call_budget_resume platform_bindings_contract sector_ready_order_contract
        vsync_field_clock_contract widescreen_contract widescreen_stage_wedge_contract)
  add_executable(tekken3_${test_name} tests/${test_name}.cpp)
  target_link_libraries(tekken3_${test_name} PRIVATE tekken3_game psxport_test_support)
  add_test(NAME tekken3_${test_name} COMMAND tekken3_${test_name})
endforeach()

add_executable(tekken3_loader_lifecycle_contract tests/loader_lifecycle_contract.cpp)
target_link_libraries(tekken3_loader_lifecycle_contract PRIVATE tekken3_game)
add_test(NAME tekken3_loader_lifecycle_contract COMMAND tekken3_loader_lifecycle_contract "${TEKKEN3_EXE}")
set_tests_properties(tekken3_loader_lifecycle_contract PROPERTIES SKIP_RETURN_CODE 77)

# IRQ oracle: the vendored Mednafen IRQ controller in its own process.
add_executable(
  tekken3_irq_oracle
  tests/irq_oracle.cpp
  "${PSXPORT_DIR}/vendor/beetle-psx/mednafen/psx/irq.c")
target_include_directories(
  tekken3_irq_oracle PRIVATE
  "${PSXPORT_DIR}/vendor/beetle-psx"
  "${PSXPORT_DIR}/vendor/beetle-psx/mednafen"
  "${PSXPORT_DIR}/vendor/beetle-psx/mednafen/psx"
  "${PSXPORT_DIR}/vendor/beetle-psx/include"
  "${PSXPORT_DIR}/vendor/beetle-psx/libretro-common/include")
target_compile_features(tekken3_irq_oracle PRIVATE cxx_std_20)
add_test(NAME tekken3_irq_oracle_selftest COMMAND tekken3_irq_oracle --selftest)

# Diffs the constants in game/fieldclock/field_clock.h against the authenticated executable.
add_test(
  NAME tekken3_vsync_field_clock
  COMMAND
    "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_vsync_field_clock.py"
    --exe "${TEKKEN3_EXE}")
add_test(
  NAME tekken3_launcher_selftest
  COMMAND
    "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_run.py")
add_test(
  NAME tekken3_launcher_help_contract
  COMMAND
    "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_launcher_help.py")
add_custom_target(
  cpp_policy
  COMMAND
    "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/check_cpp_style.py"
    --root "${CMAKE_CURRENT_SOURCE_DIR}"
    --compile-commands "${CMAKE_BINARY_DIR}"
  DEPENDS tekken3_game tekken3_port
  USES_TERMINAL
  VERBATIM)
