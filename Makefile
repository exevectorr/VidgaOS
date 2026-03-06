# VidgaOS Makefile - GRUB-based kernel build

# Compiler and toolchain
CC := gcc
AS := nasm
LD := ld
OBJCOPY := objcopy

# Flags  
CFLAGS := -ffreestanding -Wall -Wextra -fno-builtin -fno-stack-protector -m32 -march=i386 
ASFLAGS := -f elf32
LDFLAGS := -T linker.ld -m elf_i386 --oformat=elf32-i386

# Directories
SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)

# Output files
KERNEL_ELF := $(BIN_DIR)/kernel.elf
ISO_NAME := vidgaos.iso

# Find all C and assembly source files recursively
C_SOURCES := $(shell find $(SRC_DIR) -name '*.c' -type f)
ENTRY_SOURCE := $(SRC_DIR)/entry.s
OTHER_S_SOURCES := $(filter-out $(ENTRY_SOURCE),$(shell find $(SRC_DIR) -name '*.s' -type f))

# Generate object file names
ENTRY_OBJ := $(OBJ_DIR)/entry.o
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
S_OBJECTS := $(patsubst $(SRC_DIR)/%.s,$(OBJ_DIR)/%.o,$(OTHER_S_SOURCES))
ALL_OBJECTS := $(ENTRY_OBJ) $(C_OBJECTS) $(S_OBJECTS)

# Default target
.PHONY: all
all: $(KERNEL_ELF)

# Create build directories
$(BUILD_DIR) $(OBJ_DIR):
	@mkdir -p $@

# Assemble entry.s first
$(ENTRY_OBJ): $(ENTRY_SOURCE) | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "[AS] $<"
	@nasm -f elf32 -o $@ $<

# Compile C files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble other assembly files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "[AS] $<"
	@$(AS) $(ASFLAGS) -o $@ $<

# Link kernel ELF
$(KERNEL_ELF): $(ALL_OBJECTS) linker.ld | $(BIN_DIR)
	@echo "[LD] Linking kernel..."
	@$(LD) $(LDFLAGS) $(ALL_OBJECTS) -o $@
	@echo "[OK] Built: $(KERNEL_ELF)"
	
# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)
	@echo "Done!"

# Show help
.PHONY: help
help:
	@echo "VidgaOS Makefile Targets:"
	@echo "  make all      - Build the GRUB-compatible kernel (default)"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make help     - Show this help message"

# Display configuration
.PHONY: info
info:
	@echo "=== VidgaOS Build Configuration ==="
	@echo "C Sources: $(C_SOURCES)"
	@echo "Entry Source: $(ENTRY_SOURCE)"
	@echo "Other ASM Sources: $(OTHER_S_SOURCES)"
	@echo "All Objects: $(ALL_OBJECTS)"
	@echo "Kernel ELF: $(KERNEL_ELF)"
	@echo "===================================="

# Prevent deletion of intermediate files
.SECONDARY: $(ALL_OBJECTS)
