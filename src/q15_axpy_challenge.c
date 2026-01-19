/*
 * q15_axpy_challenge.c
 *
 * RISC-V Audiomark Challenge - Saturating AXPY
 * Computes: y[i] = clamp(a[i] + alpha * b[i]) for 16-bit signed integers
 *
 * Author: Ayush Dwivedi
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__riscv) && defined(__riscv_vector)
#include <riscv_vector.h>
#endif

// Clamp value to 16-bit signed range
static inline int16_t sat_q15(int32_t v) {
  if (v > 32767)
    return 32767;
  if (v < -32768)
    return -32768;
  return (int16_t)v;
}

// Scalar reference - this is what we need to match exactly
void q15_axpy_ref(const int16_t *a, const int16_t *b, int16_t *y, int n,
                  int16_t alpha) {
  for (int i = 0; i < n; i++) {
    int32_t result = (int32_t)a[i] + (int32_t)alpha * (int32_t)b[i];
    y[i] = sat_q15(result);
  }
}

// RVV vectorized version
#if defined(__riscv) && defined(__riscv_vector)

void q15_axpy_rvv(const int16_t *a, const int16_t *b, int16_t *y, int n,
                  int16_t alpha) {
  size_t remaining = (size_t)n;

  while (remaining > 0) {
    size_t vl = __riscv_vsetvl_e16m1(remaining);

    // Load chunks of a and b
    vint16m1_t va = __riscv_vle16_v_i16m1(a, vl);
    vint16m1_t vb = __riscv_vle16_v_i16m1(b, vl);

    // Widen to 32-bit, multiply, then add
    vint32m2_t prod = __riscv_vwmul_vx_i32m2(vb, alpha, vl);
    vint32m2_t sum = __riscv_vwadd_wv_i32m2(prod, va, vl);

    // Saturate and narrow in one instruction (vnclip)
    // Shift=0 means just saturate to int16 range, no actual shifting
    vint16m1_t vy = __riscv_vnclip_wx_i16m1(sum, 0, __RISCV_VXRM_RNU, vl);
    __riscv_vse16_v_i16m1(y, vy, vl);

    a += vl;
    b += vl;
    y += vl;
    remaining -= vl;
  }
}

#else

// Fallback when not on RISC-V
void q15_axpy_rvv(const int16_t *a, const int16_t *b, int16_t *y, int n,
                  int16_t alpha) {
  q15_axpy_ref(a, b, y, n, alpha);
}

#endif

// Check if two arrays match
static int arrays_match(const int16_t *ref, const int16_t *test, int n,
                        int32_t *max_diff) {
  int match = 1;
  *max_diff = 0;

  for (int i = 0; i < n; i++) {
    int32_t diff = (int32_t)ref[i] - (int32_t)test[i];
    if (diff < 0)
      diff = -diff;
    if (diff > *max_diff)
      *max_diff = diff;
    if (diff != 0)
      match = 0;
  }
  return match;
}

#if defined(__riscv)
static inline uint64_t rdcycle(void) {
  uint64_t c;
  asm volatile("rdcycle %0" : "=r"(c));
  return c;
}
#else
static inline uint64_t rdcycle(void) { return 0; }
#endif

// Quick sanity checks for edge cases
static int run_edge_tests(void) {
  int16_t a, b, y_ref, y_rvv;
  int pass = 1;

  printf("Edge case tests:\n");

  // Overflow: 32767 + 1 should clamp to 32767
  a = 32767;
  b = 1;
  q15_axpy_ref(&a, &b, &y_ref, 1, 1);
  q15_axpy_rvv(&a, &b, &y_rvv, 1, 1);
  printf("  Overflow: %s\n",
         (y_ref == 32767 && y_rvv == 32767) ? "ok" : "FAIL");
  if (y_ref != 32767 || y_rvv != 32767)
    pass = 0;

  // Underflow: -32768 + (-1) should clamp to -32768
  a = -32768;
  b = 1;
  q15_axpy_ref(&a, &b, &y_ref, 1, -1);
  q15_axpy_rvv(&a, &b, &y_rvv, 1, -1);
  printf("  Underflow: %s\n",
         (y_ref == -32768 && y_rvv == -32768) ? "ok" : "FAIL");
  if (y_ref != -32768 || y_rvv != -32768)
    pass = 0;

  // Big product: 0 + 32767*32767 should clamp
  a = 0;
  b = 32767;
  q15_axpy_ref(&a, &b, &y_ref, 1, 32767);
  q15_axpy_rvv(&a, &b, &y_rvv, 1, 32767);
  printf("  Big positive: %s\n",
         (y_ref == 32767 && y_rvv == 32767) ? "ok" : "FAIL");
  if (y_ref != 32767 || y_rvv != 32767)
    pass = 0;

  // Normal case: 100 + 5*200 = 1100, no clamping needed
  a = 100;
  b = 200;
  q15_axpy_ref(&a, &b, &y_ref, 1, 5);
  q15_axpy_rvv(&a, &b, &y_rvv, 1, 5);
  printf("  Normal case: %s\n",
         (y_ref == 1100 && y_rvv == 1100) ? "ok" : "FAIL");
  if (y_ref != 1100 || y_rvv != 1100)
    pass = 0;

  printf("\n");
  return pass;
}

int main(void) {
  int ok = 1;
  const int N = 4096;

  if (!run_edge_tests())
    ok = 0;

  // Allocate test buffers
  int16_t *a = aligned_alloc(64, N * sizeof(int16_t));
  int16_t *b = aligned_alloc(64, N * sizeof(int16_t));
  int16_t *y_ref = aligned_alloc(64, N * sizeof(int16_t));
  int16_t *y_rvv = aligned_alloc(64, N * sizeof(int16_t));

  if (!a || !b || !y_ref || !y_rvv) {
    fprintf(stderr, "alloc failed\n");
    return 1;
  }

  // Fill with random data
  srand(1234);
  for (int i = 0; i < N; i++) {
    a[i] = (int16_t)(rand() % 65536 - 32768);
    b[i] = (int16_t)(rand() % 65536 - 32768);
  }

  int16_t alpha = 3;
  int32_t max_diff;

  printf("Benchmark (N=%d, alpha=%d)\n\n", N, alpha);

  // Time scalar version
  uint64_t t0 = rdcycle();
  q15_axpy_ref(a, b, y_ref, N, alpha);
  uint64_t t1 = rdcycle();
  printf("Scalar: %llu cycles (%.2f per element)\n",
         (unsigned long long)(t1 - t0), (double)(t1 - t0) / N);

  // Time vector version
  t0 = rdcycle();
  q15_axpy_rvv(a, b, y_rvv, N, alpha);
  t1 = rdcycle();

  int match = arrays_match(y_ref, y_rvv, N, &max_diff);
  if (!match)
    ok = 0;

  printf("RVV:    %llu cycles (%.2f per element)\n",
         (unsigned long long)(t1 - t0), (double)(t1 - t0) / N);
  printf("Verify: %s (max diff = %d)\n", match ? "PASS" : "FAIL", max_diff);

  free(a);
  free(b);
  free(y_ref);
  free(y_rvv);

  return ok ? 0 : 1;
}
