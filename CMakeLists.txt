cmake_minimum_required(VERSION 3.16)

project(jala
    VERSION 1.0.0
    DESCRIPTION "Persian (Jalali) calendar in the terminal"
    LANGUAGES CXX
)

# ---- استاندارد C++ ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---- Boost ----
find_package(Boost 1.66 REQUIRED COMPONENTS date_time)

# ---- فایل اجرایی ----
add_executable(jala jala.cpp)
target_link_libraries(jala PRIVATE Boost::date_time)

# ---- هشدارهای کامپایلر ----
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(jala PRIVATE -Wall -Wextra)
endif()

# ---- نصب ----
include(GNUInstallDirs)

install(TARGETS jala
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES jala.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

# ---- تست‌ها ----
enable_testing()

add_test(
    NAME jala_tests
    COMMAND ${CMAKE_COMMAND} -E env
            JALA_BIN=$<TARGET_FILE:jala>
            bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
