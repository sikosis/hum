cmake_minimum_required(VERSION 3.16)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" HUM_VERSION LIMIT_COUNT 1)
project(hum VERSION "${HUM_VERSION}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(hum
    src/main.cpp
    src/style.cpp
    src/confirm.cpp
    src/join.cpp
    src/log.cpp
    src/input.cpp
    src/choose.cpp
    src/spin.cpp
    src/pager.cpp
    src/filter.cpp
    src/file.cpp
    src/table.cpp
    src/write.cpp
    src/viewport.cpp
    src/terminal.cpp
    src/text.cpp
)

target_include_directories(hum PRIVATE include)
target_compile_definitions(hum PRIVATE HUM_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hum PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
install(TARGETS hum RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()

add_executable(hum-terminal-tests
    tests/terminal_test.cpp
    src/terminal.cpp
)
target_include_directories(hum-terminal-tests PRIVATE include)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hum-terminal-tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(
    NAME hum-cli
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.sh $<TARGET_FILE:hum>
)
add_test(NAME hum-terminal COMMAND hum-terminal-tests)
