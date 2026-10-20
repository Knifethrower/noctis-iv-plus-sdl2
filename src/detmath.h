#pragma once

// sin, cos, atan and pow from musl 1.2.5 (src/detmath, MIT), built without
// fused multiply-add: every CPU and C library gives the same results, so the
// worlds look the same on PC and handheld (glibc's atan differs between them).

#ifdef __cplusplus
extern "C" {
#endif
double nm_sin(double);
double nm_cos(double);
double nm_atan(double);
double nm_pow(double, double);
#ifdef __cplusplus
}
#endif

#define sin nm_sin
#define cos nm_cos
#define atan nm_atan
#define pow nm_pow
