cmake_minimum_required(VERSION 3.9)

project(noctis CXX)

find_package(SDL2 REQUIRED)

set(SOURCE_FILES src/noctis.cpp src/noctis-1.cpp src/noctis-0.cpp src/brtl.cpp src/platform.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)

# The DOS code relies on signed char, wrapping integer math and type punning;
# keep x86 behavior on ARM (no fused multiply-add either: same worlds).
target_compile_options(${PROJECT_NAME} PRIVATE -fsigned-char -fwrapv -fno-strict-aliasing
                       -ffp-contract=off)
