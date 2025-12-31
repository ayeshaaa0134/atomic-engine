cmake_minimum_required(VERSION 3.10)
project(AtomicTreeEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

include_directories(include)

# Check for Windows for PDH
if(WIN32)
    add_definitions(-D_WIN32)
    # Link Pdh for metrics
    set(OS_LIBS Pdh)
else()
    set(OS_LIBS pthread)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(atomic-engine ${SOURCES})

target_link_libraries(atomic-engine PRIVATE ${OS_LIBS})

# Test Executable
add_executable(atomic-tests tests/stress_test.cpp src/b_tree.cpp src/allocator.cpp src/primitives.cpp src/manager.cpp)
target_link_libraries(atomic-tests PRIVATE ${OS_LIBS})
