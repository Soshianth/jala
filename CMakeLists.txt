cmake_minimum_required(VERSION 3.16)

project(jala
    VERSION 1.3.0
    DESCRIPTION "Persian (Jalali) calendar in the terminal"
    LANGUAGES CXX
)

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---- Python (needed for data generation) ----
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# ---- Generated data files ----
# src/holidays_data.hpp and data/overrides.json are derived from
# data/holidays.json by the Python scripts under scripts/. They are
# not tracked in version control; CMake regenerates them whenever a
# source file or generator changes.

set(GEN_OVERRIDES "${CMAKE_CURRENT_SOURCE_DIR}/data/overrides.json")
set(GEN_HPP       "${CMAKE_CURRENT_SOURCE_DIR}/src/holidays_data.hpp")

set(JSON_SRC       "${CMAKE_CURRENT_SOURCE_DIR}/data/holidays.json")
set(JSON_MANUAL    "${CMAKE_CURRENT_SOURCE_DIR}/data/manual_overrides.json")
set(PY_NEUTRALIZE  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/neutralize_overrides.py")
set(PY_EMBED       "${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_holidays.py")

add_custom_command(
    OUTPUT  ${GEN_OVERRIDES}
    COMMAND ${Python3_EXECUTABLE} ${PY_NEUTRALIZE}
    DEPENDS ${JSON_SRC} ${JSON_MANUAL} ${PY_NEUTRALIZE}
    COMMENT "Generating data/overrides.json"
    VERBATIM
)

add_custom_command(
    OUTPUT  ${GEN_HPP}
    COMMAND ${Python3_EXECUTABLE} ${PY_EMBED}
    DEPENDS ${JSON_SRC} ${GEN_OVERRIDES} ${PY_EMBED}
    COMMENT "Generating src/holidays_data.hpp"
    VERBATIM
)

# A custom target so users can run `cmake --build build --target holidays`
# to regenerate the data files by hand.
add_custom_target(holidays DEPENDS ${GEN_HPP})

# ---- Source files ----
add_executable(jala
    src/main.cpp
    src/jalali.cpp
    src/gregorian.cpp
    src/holidays.cpp
    ${GEN_HPP}
)

# Ensure the generated header is produced before the build starts.
add_dependencies(jala holidays)

target_include_directories(jala PRIVATE src)

# ---- Compiler warnings ----
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(jala PRIVATE -Wall -Wextra)
endif()

# ---- Install ----
include(GNUInstallDirs)

install(TARGETS jala
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES jala.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

# ---- Tests ----
enable_testing()

add_test(
    NAME jala_tests
    COMMAND ${CMAKE_COMMAND} -E env
            JALA_BIN=$<TARGET_FILE:jala>
            bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
    NAME jala_secularization
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/secularization_test.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)