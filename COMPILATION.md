# Compiling and Using VidgaOS

VidgaOS is a simple operating system written in C and assembly. This guide explains how to compile the kernel, create a bootable ISO, and run it in a virtual machine.

## Prerequisites

Before compiling VidgaOS, ensure you have the following tools installed:

- **GCC**: For compiling C code (`gcc`)
- **NASM**: For assembling assembly code (`nasm`)
- **Binutils**: For linking (`ld`)
- **GRUB tools**: For creating bootable ISOs (`grub-mkrescue`) or alternatively `xorriso`
- **QEMU**: For running the OS in a virtual machine (`qemu-system-i386`)

### Installation on Windows

If you're on Windows, you can use WSL (Windows Subsystem for Linux) or MSYS2/MinGW to get these tools.

#### Using WSL (Recommended for windows)

1. Install WSL from Microsoft Store (Ubuntu recommended)
2. Open WSL terminal and install tools:
   ```bash
   sudo apt update
   sudo apt install build-essential nasm qemu-system-x86 grub-pc-bin xorriso
   ```

#### Using MSYS2

1. Download and install MSYS2 from https://www.msys2.org/
2. Open MSYS2 terminal and install:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-nasm mingw-w64-x86_64-binutils mingw-w64-x86_64-grub mingw-w64-x86_64-qemu
   ```

## Building the Kernel

1. Open a terminal in the project directory 

2. Build the kernel:
   ```bash
   make
   ```
   This will compile all C and assembly source files and link them into `build/kernel.elf`.

## Creating the Bootable ISO

1. After building the kernel, create the ISO:
   ```bash
   ./build-iso.sh
   ```
   This script will:
   - Check if the kernel exists
   - Create a temporary ISO structure
   - Copy the kernel and GRUB configuration
   - Generate `vidgaos.iso` using GRUB tools

## Creating a Hard Disk Image

VidgaOS requires a FAT32-formatted hard disk for filesystem operations. Create one with:

```bash
# Create a 10MB disk image
dd if=/dev/zero of=disk.img bs=1M count=10

# Format it as FAT32
mkfs.vfat -F 32 disk.img
```

## Running VidgaOS

To run the OS in QEMU with filesystem support:

```bash
qemu-system-i386 -drive file=disk.img,format=raw,if=ide,index=0 -drive file=vidgaos.iso,media=cdrom,if=ide,index=1 -boot d
```

This configures:
- `disk.img` as IDE drive 0 (hard disk with FAT32 filesystem)
- `vidgaos.iso` as IDE drive 1 (CD-ROM with the OS)
- Boots from the CD-ROM

You should see:
- GRUB menu briefly
- "VidgaOS running in 32bit mode"
- "FAT32: Initialized successfully"
- The shell prompt

### Alternative QEMU Options

- For more memory: Add `-m 512`
- For debugging: Add `-s -S` (then attach gdb)
- To enable serial output: Add `-serial stdio`
- Without hard disk (limited functionality): `qemu-system-i386 -cdrom vidgaos.iso`

## Troubleshooting

### Build Errors

- **Missing tools**: Ensure all prerequisites are installed and in PATH
- **Assembly errors**: Check that NASM is version 2.x or later
- **Linking errors**: Verify linker.ld exists and is correct

### ISO Creation Errors

- **grub-mkrescue not found**: Install GRUB tools or use xorriso as fallback
- **Permission denied**: Run the script with execute permissions: `chmod +x build-iso.sh`

### Runtime Issues

- **"Booting from hard disk" error**: Use the correct QEMU command with proper drive ordering
- **FAT32 errors**: Ensure you have a FAT32-formatted hard disk image attached
- **"Invalid sector size"**: The hard disk must be drive 0; use the specified QEMU command
- **No boot**: Check that the ISO was created successfully and uses Multiboot (not Multiboot2)
- **Kernel panic**: The OS may have bugs; check kernel.c for issues
- **No display**: Ensure QEMU is using the correct graphics mode

## Project Structure

- `src/`: Source code
  - `kernel.c`: Main kernel code
  - `entry.s`: Assembly entry point
  - `drivers/`: Hardware drivers (VGA, keyboard)
  - `kernel/`: Kernel subsystems (ATA, FAT32, VFS, etc.)
  - `shell/`: Command shell
  - `games/`: Simple games
- `Makefile`: Build configuration
- `build-iso.sh`: ISO creation script
- `grub.cfg`: GRUB bootloader configuration
- `linker.ld`: Linker script

## Contributing

If you modify the code, remember to:
1. Rebuild with `make`
2. Recreate ISO with `./build-iso.sh`
3. Test in QEMU

For more details on specific components, see the other documentation files like `FAT32_IMPLEMENTATION.md` and `HIERARCHICAL_FILESYSTEM.md`.