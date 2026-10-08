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

# ---- Source files ----
# The Gregorian calendar is implemented locally in src/gregorian.cpp,
# so there is no external dependency (in particular, no Boost).
add_executable(jala
    src/main.cpp
    src/jalali.cpp
    src/gregorian.cpp
    src/holidays.cpp
)

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