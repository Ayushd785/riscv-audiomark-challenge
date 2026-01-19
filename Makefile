# RISC-V Q15 AXPY Challenge - Makefile
# Supports cross-compilation for RV32 and RV64 targets

# Toolchain prefix (adjust if your toolchain has different naming)
RV32_PREFIX ?= riscv32-unknown-elf-
RV64_PREFIX ?= riscv64-unknown-elf-

# Compiler flags
COMMON_FLAGS = -O2 -Wall -Wextra -Iinclude
RV32_FLAGS = -march=rv32imcv -mabi=ilp32
RV64_FLAGS = -march=rv64gcv -mabi=lp64d

# Source files
SRC = src/q15_axpy_challenge.c

# Output directory
BUILD_DIR = build

# Targets
.PHONY: all rv32 rv64 clean help

all: rv64

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build for RV32
rv32: $(BUILD_DIR)
	$(RV32_PREFIX)gcc $(COMMON_FLAGS) $(RV32_FLAGS) $(SRC) -o $(BUILD_DIR)/q15_axpy_rv32.elf
	@echo "Built: $(BUILD_DIR)/q15_axpy_rv32.elf"

# Build for RV64
rv64: $(BUILD_DIR)
	$(RV64_PREFIX)gcc $(COMMON_FLAGS) $(RV64_FLAGS) $(SRC) -o $(BUILD_DIR)/q15_axpy_rv64.elf
	@echo "Built: $(BUILD_DIR)/q15_axpy_rv64.elf"

# Build both
both: rv32 rv64

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Show help
help:
	@echo "RISC-V Q15 AXPY Challenge Build System"
	@echo ""
	@echo "Targets:"
	@echo "  rv32   - Build for RV32 (requires riscv32-unknown-elf-gcc)"
	@echo "  rv64   - Build for RV64 (requires riscv64-unknown-elf-gcc)"
	@echo "  both   - Build for both RV32 and RV64"
	@echo "  clean  - Remove build artifacts"
	@echo ""
	@echo "Variables:"
	@echo "  RV32_PREFIX - Toolchain prefix for RV32 (default: riscv32-unknown-elf-)"
	@echo "  RV64_PREFIX - Toolchain prefix for RV64 (default: riscv64-unknown-elf-)"
	@echo ""
	@echo "Example:"
	@echo "  make rv64 RV64_PREFIX=riscv64-linux-gnu-"
