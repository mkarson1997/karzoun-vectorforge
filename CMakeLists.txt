cmake_minimum_required(VERSION 3.24)
project(KarzounVectorForge VERSION 0.1.0 LANGUAGES CXX)

option(VECTORFORGE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(VECTORFORGE_BUILD_BENCHMARKS "Build benchmark executable" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_library(vectorforge_core
    src/distance.cpp
    src/exact_index.cpp
    src/graph_index.cpp
)
target_include_directories(vectorforge_core PUBLIC include)
target_link_libraries(vectorforge_core PUBLIC Threads::Threads)

target_compile_options(vectorforge_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;-Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX;/permissive->
)

if(VECTORFORGE_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(vectorforge_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(vectorforge_core PUBLIC -fsanitize=address,undefined)
    else()
        message(FATAL_ERROR "Sanitizer configuration currently supports GCC and Clang")
    endif()
endif()

add_executable(vectorforge src/main.cpp)
target_link_libraries(vectorforge PRIVATE vectorforge_core)

if(VECTORFORGE_BUILD_BENCHMARKS)
    add_executable(vectorforge_benchmark bench/benchmark.cpp)
    target_link_libraries(vectorforge_benchmark PRIVATE vectorforge_core)
endif()

enable_testing()
add_executable(vectorforge_tests tests/test_vectorforge.cpp)
target_link_libraries(vectorforge_tests PRIVATE vectorforge_core)
add_test(NAME vectorforge_tests COMMAND vectorforge_tests)
