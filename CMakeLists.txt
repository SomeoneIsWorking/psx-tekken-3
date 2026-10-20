cmake_minimum_required(VERSION 3.24)
project(tekken3_port LANGUAGES C CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Framework checkout: a symlink or clone set up by psxport's tools/psxport_fetch.py.
set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: external/psxport)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — run "
                      "./run.sh once to fetch it, or point PSXPORT_DIR at a psxport checkout.")
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

# The title's owners; the product links them.
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
