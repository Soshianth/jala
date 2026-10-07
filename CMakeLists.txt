cmake_minimum_required(VERSION 3.16)

project(jala
    VERSION 1.2.0
    DESCRIPTION "Persian (Jalali) calendar in the terminal"
    LANGUAGES CXX
)

# CMake 3.30+ removed the FindBoost module in favor of Boost's own
# BoostConfig.cmake. Suppress the associated deprecation warning when
# a modern CMake is used.
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---- Boost ----
find_package(Boost 1.66 REQUIRED CONFIG COMPONENTS date_time)

# ---- Source files ----
add_executable(jala
    src/main.cpp
    src/jalali.cpp
    src/holidays.cpp
)

target_include_directories(jala PRIVATE src)
target_link_libraries(jala PRIVATE Boost::date_time)

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