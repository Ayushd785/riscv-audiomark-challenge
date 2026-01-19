/*
 * q15_axpy.h - Q15 Saturating AXPY for RISC-V
 */

#ifndef Q15_AXPY_H
#define Q15_AXPY_H

#include <stdint.h>

// Scalar reference (golden)
void q15_axpy_ref(const int16_t *a, const int16_t *b, int16_t *y, int n,
                  int16_t alpha);

// RVV vectorized version
void q15_axpy_rvv(const int16_t *a, const int16_t *b, int16_t *y, int n,
                  int16_t alpha);

#endif
