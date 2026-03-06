#!/bin/bash

# Build a GRUB bootable ISO image

set -e

KERNEL="build/kernel.elf"
OUTPUT="vidgaos.iso"

if [ ! -f "$KERNEL" ]; then
    echo "Error: kernel not found. Run 'make' first."
    exit 1
fi

# Create temporary directories for ISO
TMP_DIR=$(mktemp -d)
ISODIR="$TMP_DIR/iso"
BOOTDIR="$ISODIR/boot"
GRUBDIR="$BOOTDIR/grub"

mkdir -p "$GRUBDIR"

# Copy kernel to ISO boot directory
cp "$KERNEL" "$BOOTDIR/kernel.elf"

# Copy grub config
cp grub.cfg "$GRUBDIR/"

# Create a FAT32 disk image for the OS to use
DISK_IMG="disk.img"
DISK_SIZE_MB=10

if command -v mkfs.fat &> /dev/null; then
    echo "Creating FAT32 disk image ($DISK_SIZE_MB MB)..."
    dd if=/dev/zero of="$DISK_IMG" bs=1M count=$DISK_SIZE_MB
    mkfs.fat -F 32 "$DISK_IMG"
else
    echo "Warning: mkfs.fat not found. Disk image not created."
    echo "Install dosfstools: sudo apt install dosfstools"
fi

# Create ISO using grub-mkrescue (if available) or fallback
if command -v grub-mkrescue &> /dev/null; then
    echo "Creating bootable ISO with grub-mkrescue..."
    grub-mkrescue -o "$OUTPUT" "$ISODIR"
elif command -v xorriso &> /dev/null; then
    echo "Creating bootable ISO with xorriso..."
    xorriso -as mkisofs -R -b boot/grub/i386-pc/eltorito.img \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        -o "$OUTPUT" "$ISODIR"
else
    echo "Error: Neither grub-mkrescue nor xorriso found."
    echo "Install GRUB tools: sudo pacman -S grub  (Arch)"
    echo "                    sudo apt install grub-pc-bin xorriso  (Debian/Ubuntu)"
    rm -rf "$TMP_DIR"
    exit 1
fi

# Cleanup
rm -rf "$TMP_DIR"

echo "ISO created: $OUTPUT"
if [ -f "$DISK_IMG" ]; then
    echo "Disk image created: $DISK_IMG"
    echo ""
    echo "To test with QEMU:"
    echo "    qemu-system-i386 -cdrom $OUTPUT -hda $DISK_IMG -boot d"
else
    echo ""
    echo "To test with QEMU:"
    echo "    qemu-system-i386 -cdrom $OUTPUT"
    echo "Note: No disk image created. Filesystem commands may not work."
fi
