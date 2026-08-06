#!/usr/bin/env python
# AIkFS image builder — bakes a staging directory into a disk image partition.
# Format per ADR-015 / Phase-3-Memory-and-Files.md (512-byte blocks).
import os
import struct
import sys

BLOCK_SIZE = 512
MAGIC = b'AIkFS1'
VERSION = 1

def u32_le(x):
    return struct.pack('<I', x)

def u64_le(x):
    return struct.pack('<Q', x)

def read_u32_le(data, offset):
    return struct.unpack('<I', data[offset:offset+4])[0]

def read_u64_le(data, offset):
    return struct.unpack('<Q', data[offset:offset+8])[0]

class AIkFSBuilder:
    def __init__(self, part_lba, part_sectors):
        self.part_lba = part_lba
        self.part_sectors = part_sectors
        self.block_count = part_sectors
        self.bitmap_start = 1
        self.bitmap_blocks = 1  # 1 block = 512 bytes = 4096 bits, enough for 4096 blocks
        self.root_dir_block = 2
        self.bin_dir_block = 3
        
        # Track used blocks
        self.used_blocks = [False] * self.block_count
        self.used_blocks[0] = True  # superblock
        self.used_blocks[1] = True  # bitmap
        self.used_blocks[2] = True  # root dir
        self.used_blocks[3] = True  # bin dir
        
        self.next_data_block = 4  # Data blocks start at block 4 (after bin dir at block 3)
        
        self.files = []  # (name, size, first_block, block_count, data)
        self.bin_entries = []  # entries for bin directory

    def add_file(self, name, data):
        size = len(data)
        block_count = (size + BLOCK_SIZE - 1) // BLOCK_SIZE
        first_block = self.next_data_block
        
        if first_block + block_count > self.block_count:
            raise ValueError(f"File {name} ({size} bytes, {block_count} blocks) does not fit in partition")
        
        # Mark blocks as used
        for i in range(block_count):
            self.used_blocks[first_block + i] = True
        
        self.next_data_block = first_block + block_count
        self.files.append((name, size, first_block, block_count, data))
        self.bin_entries.append((name, size, first_block, block_count))

    def build_bitmap(self):
        bitmap = bytearray(BLOCK_SIZE)
        for i, used in enumerate(self.used_blocks):
            if used:
                byte_idx = i // 8
                bit_idx = i % 8
                bitmap[byte_idx] |= (1 << bit_idx)
        return bytes(bitmap)

    def build_superblock(self):
        sb = bytearray(BLOCK_SIZE)
        sb[0:6] = MAGIC
        sb[6] = VERSION
        sb[8:12] = u32_le(self.block_count)
        sb[12:16] = u32_le(self.bitmap_start)
        sb[16:20] = u32_le(self.bitmap_blocks)
        sb[20:24] = u32_le(self.root_dir_block)
        return bytes(sb)

    def build_dir_entry(self, name, type_, size, first_block, block_count):
        entry = bytearray(32)
        # name[16] NUL-padded
        name_bytes = name.encode('ascii')
        if len(name_bytes) > 16:
            raise ValueError(f"Filename too long: {name}")
        entry[0:len(name_bytes)] = name_bytes
        entry[16] = type_  # 1=file, 2=dir
        entry[17:21] = u32_le(size)
        entry[21:25] = u32_le(first_block)
        entry[25:29] = u32_le(block_count)
        # bytes 29-31 remain zero
        return bytes(entry)

    def build_root_dir(self):
        root = bytearray(BLOCK_SIZE)
        # Root contains exactly one entry: 'bin' (type 2, size 512, first_block 3, block_count 1)
        entry = self.build_dir_entry('bin', 2, 512, 3, 1)
        root[0:32] = entry
        return bytes(root)

    def build_bin_dir(self):
        bin_dir = bytearray(BLOCK_SIZE)
        offset = 0
        for name, size, first_block, block_count in self.bin_entries:
            entry = self.build_dir_entry(name, 1, size, first_block, block_count)
            bin_dir[offset:offset+32] = entry
            offset += 32
        return bytes(bin_dir)

    def build_partition(self):
        partition = bytearray(self.block_count * BLOCK_SIZE)
        
        # Block 0: superblock
        partition[0:BLOCK_SIZE] = self.build_superblock()
        
        # Block 1: bitmap
        partition[BLOCK_SIZE:2*BLOCK_SIZE] = self.build_bitmap()
        
        # Block 2: root directory
        partition[2*BLOCK_SIZE:3*BLOCK_SIZE] = self.build_root_dir()
        
        # Block 3: bin directory
        partition[3*BLOCK_SIZE:4*BLOCK_SIZE] = self.build_bin_dir()
        
        # Data blocks (starting at block 4)
        for name, size, first_block, block_count, data in self.files:
            start = first_block * BLOCK_SIZE
            partition[start:start+size] = data
        
        return bytes(partition)

def main():
    if len(sys.argv) != 5:
        print(f"Usage: {sys.argv[0]} <disk_img> <part_lba> <part_sectors> <staging_dir>")
        sys.exit(1)
    
    disk_img = sys.argv[1]
    part_lba = int(sys.argv[2])
    part_sectors = int(sys.argv[3])
    staging_dir = sys.argv[4]
    
    if not os.path.isdir(staging_dir):
        print(f"ERROR: staging directory '{staging_dir}' does not exist")
        sys.exit(1)
    
    # Read existing disk image
    with open(disk_img, 'rb') as f:
        disk = bytearray(f.read())
    
    # Ensure disk is large enough
    required_size = (part_lba + part_sectors) * BLOCK_SIZE
    if len(disk) < required_size:
        disk.extend(b'\x00' * (required_size - len(disk)))
    
    # Build AIkFS partition
    builder = AIkFSBuilder(part_lba, part_sectors)
    
    # Add all files from staging_dir
    files_added = 0
    for fname in sorted(os.listdir(staging_dir)):
        fpath = os.path.join(staging_dir, fname)
        if os.path.isfile(fpath):
            with open(fpath, 'rb') as f:
                data = f.read()
            builder.add_file(fname, data)
            files_added += 1
    
    partition_data = builder.build_partition()
    
    # Write partition into disk image
    part_offset = part_lba * BLOCK_SIZE
    disk[part_offset:part_offset + len(partition_data)] = partition_data
    
    # Write back
    with open(disk_img, 'wb') as f:
        f.write(disk)
    
    # Self-verify: re-read the partition
    with open(disk_img, 'rb') as f:
        f.seek(part_offset)
        verify_data = f.read(part_sectors * BLOCK_SIZE)
    
    # Verify magic and version
    if verify_data[0:6] != MAGIC:
        print(f"ERROR: Magic mismatch at LBA {part_lba}")
        sys.exit(1)
    if verify_data[6] != VERSION:
        print(f"ERROR: Version mismatch at LBA {part_lba}")
        sys.exit(1)
    
    # Count data blocks used (blocks 4 onwards that are marked used in bitmap)
    bitmap = verify_data[BLOCK_SIZE:2*BLOCK_SIZE]
    data_blocks_used = 0
    for i in range(4, part_sectors):
        byte_idx = i // 8
        bit_idx = i % 8
        if bitmap[byte_idx] & (1 << bit_idx):
            data_blocks_used += 1
    
    # Print summary line
    print(f"magic={MAGIC.decode()} version={VERSION} files={files_added} data_blocks={data_blocks_used}")

if __name__ == '__main__':
    main()