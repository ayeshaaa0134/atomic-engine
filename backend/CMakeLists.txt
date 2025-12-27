cmake_minimum_required(VERSION 3.10)
project(AtomicTree)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Source files
set(SOURCES
    src/manager.cpp
    src/B_tree.cpp
    src/primitives.cpp
    src/garbage_collector.cpp
)

# Main executable (demo)
add_executable(AtomicTree main.cpp ${SOURCES})

# Tests
enable_testing()

add_executable(stress_test tests/stress_test.cpp ${SOURCES})
add_executable(speed_test tests/speed_test.cpp ${SOURCES})
add_executable(crash_test tests/crash_consistency_test.cpp ${SOURCES})
add_executable(viz_demo tests/viz_demo.cpp ${SOURCES})
