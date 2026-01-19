# RISC-V Q15 Saturating AXPY with RVV Intrinsics

Vectorized multiply-accumulate using RISC-V Vector Extension (RVV) intrinsics.

## Problem

Implement:
```c
void q15_axpy_rvv(const int16_t *a, const int16_t *b, int16_t *y, int n, int16_t alpha);
```

Computes: `y[i] = clamp(a[i] + alpha * b[i], -32768, 32767)`

## Design Choices

### 1. Widening Multiply (not Q15 fractional)

The reference does integer multiply, not Q15 fractional:
```c
int32_t result = (int32_t)a[i] + (int32_t)alpha * (int32_t)b[i];
```

So I use `vwmul` (widening multiply) instead of `vsmul` (fixed-point).

### 2. RVV Intrinsics Used

| Instruction | Purpose |
|-------------|---------|
| `vsetvl_e16m1` | Set vector length (LMUL=1) |
| `vle16` | Load 16-bit vectors |
| `vwmul.vx` | 16×16 → 32-bit multiply |
| `vwadd.wv` | 32-bit + 16-bit(widened) add |
| `vmax/vmin` | Clamp to [-32768, 32767] |
| `vnsra` | Narrow back to 16-bit |
| `vse16` | Store result |

### 3. Why LMUL=1?

- Simple register usage (m1 for 16-bit, m2 for 32-bit widened)
- Portable across different VLEN implementations
- Leaves headroom for the widened intermediate values

### 4. Vector-Length Agnostic

Standard strip-mining loop works with any hardware VLEN:
```c
while (remaining > 0) {
    size_t vl = __riscv_vsetvl_e16m1(remaining);
    // process vl elements
    remaining -= vl;
}
```

## Build & Run

```bash
# Install (Fedora)
sudo dnf install qemu-user
wget https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v14.2.0-3/xpack-riscv-none-elf-gcc-14.2.0-3-linux-x64.tar.gz
tar -xzf xpack-riscv-none-elf-gcc-14.2.0-3-linux-x64.tar.gz
export PATH="$HOME/xpack-riscv-none-elf-gcc-14.2.0-3/bin:$PATH"

# Build
make rv64 RV64_PREFIX=riscv-none-elf-

# Run
qemu-riscv64 -cpu rv64,v=true build/q15_axpy_rv64.elf
```

## Results (QEMU)

```
Edge case tests:
  Overflow: ok
  Underflow: ok
  Big positive: ok
  Normal case: ok

Benchmark (N=4096, alpha=3)

Scalar: 149702 cycles (36.55 per element)
RVV:    562522 cycles (137.33 per element)
Verify: PASS (max diff = 0)
```

**Note:** RVV appears slower in QEMU because it's an emulator - each vector instruction is decoded and emulated one at a time. On real hardware, RVV would be significantly faster.

## Expected Performance (Real Hardware)

Back-of-envelope calculation for VLEN=128, LMUL=1:
- Elements per vector: 128/16 = 8
- Scalar: ~5 cycles/element (load, mul, add, clamp, store)
- Vector: ~1 cycle/element (pipelined)

| VLEN | Elements/iter | Expected Speedup |
|------|---------------|------------------|
| 128  | 8             | 4-6x             |
| 256  | 16            | 8-12x            |
| 512  | 32            | 15-20x           |

## Files

```
src/q15_axpy_challenge.c  - Implementation + tests
include/q15_axpy.h        - Header
Makefile                  - Build system
README.md                 - This file
```

## Verification

All acceptance criteria met:
- ✅ Bit-for-bit identical to scalar reference (max diff = 0)
- ✅ Vector-length agnostic (uses vsetvl)
- ✅ Buildable on RV32 and RV64

## Author

Ayush Dwivedi - January 2026
