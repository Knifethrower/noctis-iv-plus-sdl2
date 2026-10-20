cmake_minimum_required(VERSION 3.13)

project(noctis C CXX)

find_package(SDL2 REQUIRED)

set(SOURCE_FILES src/noctis.cpp src/noctis-1.cpp src/noctis-0.cpp src/brtl.cpp src/platform.cpp)

# sin, cos, atan, pow from musl: the same results on every CPU and C library.
set(DETMATH_FILES sin.c cos.c __sin.c __cos.c __rem_pio2.c __rem_pio2_large.c atan.c pow.c
    pow_data.c exp_data.c __math_oflow.c __math_uflow.c __math_xflow.c __math_invalid.c
    __math_divzero.c)
list(TRANSFORM DETMATH_FILES PREPEND src/detmath/)
add_library(detmath STATIC ${DETMATH_FILES})
target_compile_options(detmath PRIVATE -ffp-contract=off -U__FP_FAST_FMA -fno-builtin
                       -w)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} detmath ${SDL2_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)

# The DOS code relies on signed char, wrapping integer math and type punning;
# keep x86 behavior on ARM (no fused multiply-add either: same worlds).
target_compile_options(${PROJECT_NAME} PRIVATE -fsigned-char -fwrapv -fno-strict-aliasing
                       -ffp-contract=off)
