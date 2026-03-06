## Complete Hierarchical Filesystem Implementation for VidgaOS

This document describes the complete filesystem implementation including directory navigation, file operations, and the virtual filesystem layer.

---

## Architecture Overview

The filesystem consists of three layers:

```
┌─────────────────────────────────────────┐
│       Shell Commands (ls, cd, mkdir)    │
├─────────────────────────────────────────┤
│   VFS Layer (vfs.c/h)                   │
│   - Path resolution & navigation        │
│   - Directory traversal                 │
│   - Relative/absolute path handling     │
├─────────────────────────────────────────┤
│   FAT32 Driver (fat32.c/h)              │
│   - FAT table management                │
│   - Cluster allocation & freeing        │
│   - Disk I/O operations                 │
├─────────────────────────────────────────┤
│   ATA/IDE Driver (ata.c/h)              │
│   - Sector read/write                   │
│   - LBA addressing                      │
└─────────────────────────────────────────┘
```

---

## Layer 1: ATA/IDE Driver (`ata.h/c`)

**Functions:**
- `ata_init()` - Initialize ATA controller
- `ata_read_sectors()` - LBA sector reading
- `ata_write_sectors()` - LBA sector writing
- `ata_read_sector()` - Single sector read

No changes from previous implementation.

---

## Layer 2: FAT32 Driver Extended (`fat32.h/c`)

### Read Operations (unchanged)
- `fat32_init()` - Initialize from boot sector
- `fat32_list_dir()` - List directory
- `fat32_find_file()` - Find file in directory
- `fat32_open()` / `fat32_read()` / `fat32_close()` - File reading

### New Write Operations

#### Cluster Allocation
```c
uint32_t fat32_alloc_cluster(void)
```
- Scans FAT table for first free cluster
- Returns cluster number or 0 if full
- Used before creating files/directories

#### FAT Table Updates
```c
int fat32_update_fat_entry(uint32_t cluster, uint32_t next)
```
- Updates FAT chain for given cluster
- Writes to all FAT copies (usually 2)
- Marks EOC with 0x0FFFFFFF

#### Cluster Writing
```c
int fat32_write_cluster(uint32_t cluster, uint8_t *buffer)
```
- Writes 4096 bytes (8 sectors) to disk
- Uses data region sector calculation
- Returns success/failure

#### Directory Entry Writing
```c
int fat32_write_dirent(uint32_t cluster, uint32_t offset, fat32_dirent_t *entry)
```
- Writes single directory entry (32 bytes)
- Reads cluster, updates entry, writes back

#### Directory Creation
```c
int fat32_mkdir(uint32_t parent_cluster, const char *dirname)
```
1. Allocates new cluster for directory
2. Marks cluster as EOC in FAT
3. Creates "." and ".." entries
4. Writes cluster to disk
5. Creates entry in parent directory

**Directory Structure:**
- Entry 0: "." pointing to own cluster
- Entry 1: ".." pointing to parent cluster
- Remaining entries: files/subdirectories

#### File Creation
```c
int fat32_create_file(uint32_t parent_cluster, const char *filename)
```
- Creates empty file (start_cluster = 0)
- Adds entry to parent directory
- File can be written to later with proper cluster allocation

#### File/Dir Deletion
```c
int fat32_delete(uint32_t parent_cluster, const char *name)
```
- Marks directory entry with 0xE5 (deleted marker)
- Does NOT free clusters (no undelete)
- Does NOT update FAT chain

---

## Layer 3: Virtual File System (`vfs.h/c`)

Provides high-level path-aware filesystem operations.

### Current Working Directory
```c
typedef struct {
    uint32_t cluster;        /* Current directory cluster */
    char path[256];          /* Current path string */
} vfs_cwd_t;
```

Maintained globally and updated by `vfs_cd()`.

### Core Functions

#### Initialize VFS
```c
void vfs_init(void)
```
- Sets CWD to root cluster "/"
- Called at kernel startup

#### Get Current Directory
```c
vfs_cwd_t* vfs_get_cwd(void)
```
- Returns pointer to current directory state
- Used by shell to display prompt

#### Change Directory
```c
int vfs_cd(const char *path)
```
- Resolves path (relative or absolute)
- Navigates to target directory
- Updates CWD if successful

**Examples:**
```
cd /system       - Absolute path to /system
cd ..            - Parent directory
cd ./home        - Relative: home subdirectory
cd /home/user    - Absolute: /home/user
```

#### List Directory
```c
int vfs_ls(const char *path)
```
- Lists files in specified directory
- Default: "." (current directory)
- Shows file sizes and types

#### Create Directory
```c
int vfs_mkdir(const char *path)
```
- Creates directory at path
- Creates parent structure if needed
- Updates parent directory entry

#### Create File
```c
int vfs_create_file(const char *path)
```
- Creates empty file
- Can be written to later

#### Delete
```c
int vfs_delete(const char *path)
```
- Removes file or directory
- Marks entry as deleted

#### Find Entry
```c
int vfs_find(const char *path, vfs_entry_t *out_entry)
```
- Locates file/directory
- Returns entry info (size, cluster, type)

### Path Resolution Functions

#### Check if Absolute Path
```c
int vfs_is_absolute(const char *path)
```
- Returns 1 if path starts with "/"
- Returns 0 for relative paths

#### Resolve Path to Absolute
```c
void vfs_resolve_path(const char *relpath, char *abspath)
```
- Converts relative path to absolute
- Appends to CWD if relative
- Returns unchanged if already absolute

**Examples:**
- Current: `/home`
- Input: `../system`
- Output: `/system`

#### Get Filename from Path
```c
const char* vfs_get_filename(const char *path)
```
- Extracts filename after last "/"
- Input: `/home/user/docs/file.txt`
- Output: `file.txt`

#### Get Parent Directory
```c
void vfs_get_parent_path(const char *path, char *parent)
```
- Returns directory containing path
- Input: `/home/user/file.txt`
- Output: `/home/user`

---

## Filesystem Structure at Boot

The kernel initializes a standard directory structure:

```
/
├── system/     (System files and drivers)
├── home/       (User home directory)
└── tmp/        (Temporary files)
```

These are created automatically at boot if they don't exist.

---

## Shell Integration

### Prompt Display
The shell prompt now shows the current directory:

```
/home> ls
...
/home> cd documents
/home/documents> pwd
Current directory: /home/documents
/home/documents> cd..
/home> _
```

### Command Enhancements

#### pwd - Print Working Directory
```
> pwd
Current directory: /home
```

#### cd - Change Directory
```
> cd /system
> cd ..
> cd ./sub-folder
```

#### ls - List Directory (deprecated path argument removal)
```
> ls
Files in current directory...
```

#### mkdir - Create Directory
```
> mkdir newdir
Directory created: newdir
```

#### touch - Create Empty File
```
> touch myfile.txt
File created: myfile.txt
```

#### rm - Remove File/Directory
```
> rm myfile.txt
Deleted: myfile.txt
```

#### cat - Display File
```
> cat file.txt
File contents...
```

---

## Implementation Details

### Path Parsing Algorithm

To navigate `/home/docs/file.txt`:

1. Check if absolute (starts with "/") → YES
2. Start at root cluster
3. Split path: `home` / `docs` / `file.txt`
4. For each component:
   - Find in current directory
   - Check if it's a directory
   - Move to its cluster
5. Return final cluster

### Directory Entry Format

Each entry is 32 bytes (FAT32 standard):

```
Offset  Size  Field
0-7     8     Filename (8.3 format name)
8-10    3     Extension
11      1     Attributes (DIR, READ-ONLY, etc)
12      1     Reserved
13-14   2     Creation time
15-16   2     Creation date
17-18   2     Access date
19-20   2     High cluster (bits 16-31)
21-22   2     Write time
23-24   2     Write date
25-26   2     Low cluster (bits 0-15)
27-30   4     File size (bytes)
```

### Cluster Chain Navigation

Files/directories span multiple clusters linked via FAT:

```
Cluster 10 → FAT[10] = 11
Cluster 11 → FAT[11] = 12
Cluster 12 → FAT[12] = 0x0FFFFFFF (EOC)
```

Reading a file:
1. Get first cluster from directory entry
2. Read cluster data
3. Look up next cluster in FAT
4. Repeat until EOC marker

### Memory Layout

```
FAT32 Allocation:
├── Sector 0: Boot Sector
├── Sectors 1-N: FAT Table 1
├── Sectors N+1-M: FAT Table 2
└── Sector M+1+: Data Region
    └── Clusters 2+: File/Directory Data
```

---

## File Operations Flow

### Creating File `/home/docs/report.txt`

1. `vfs_create_file("/home/docs/report.txt")` called
2. `vfs_resolve_path()` → `/home/docs/report.txt` (already absolute)
3. `vfs_get_parent_path()` → `/home/docs`
4. `navigate_path()` starts at root, navigates to `/home/docs` cluster
5. `vfs_get_filename()` → `report.txt`
6. `fat32_create_file(docs_cluster, "report.txt")`
7. `create_dirent()` finds free entry in docs directory
8. Creates entry with start_cluster = 0, attributes = ARCHIVE
9. `fat32_write_cluster()` writes updated directory back to disk

### Creating Directory `/home/backup`

1. `vfs_mkdir("/home/backup")`
2. `navigate_path()` → `/home` cluster
3. `fat32_mkdir(home_cluster, "backup")`
4. `fat32_alloc_cluster()` → cluster 100 (free)
5. `fat32_update_fat_entry(100, 0x0FFFFFFF)` (mark as EOC)
6. Create "." and ".." entries in cluster 100
7. `fat32_write_cluster(100, ...)` writes directory data
8. `create_dirent()` adds "backup" entry to `/home`

---

## Error Handling

**Missing component in path:**
```
> cd /home/nonexistent
Error: Directory not found
```

**No free space:**
```
> mkdir newdir
FAT32: No free clusters
Error: Could not create directory
```

**File not found:**
```
> cat missing.txt
Error: File not found: missing.txt
```

---

## Limitations & Future Work

**Current Limitations:**
- No actual data writing (files stay empty)
- No recursive deletion (no `rm -r`)
- No file renaming
- No permission/ownership
- No file timestamps (fixed values)
- Simple first-fit cluster allocation

**Future Enhancements:**
1. **File Writing** - Allocate clusters, write data
2. **Copy Command** - `cp src dest` file copying
3. **Recursive Operations** - `rm -r`, `cp -r`
4. **File Attributes** - Executable, hidden, system
5. **Links** - Both hard and soft links
6. **Compression** - On-disk compression
7. **Journaling** - Crash recovery
8. **Quotas** - Per-user space limits

---

## Testing Commands

When on a running system with FAT32 disk:

```bash
# Create directories
mkdir /system
mkdir /home
mkdir /home/user
mkdir /tmp

# Change directory
cd /home/user

# Create files
touch readme.txt
touch notes.md

# List directory contents
ls

# Navigate around
cd ..
pwd
cd /system
ls

# Clean up
rm readme.txt
cd /
```

---

## Files Modified

**New Files:**
- `src/kernel/vfs.h` / `vfs.c` - Virtual filesystem layer (450+ lines)

**Modified Files:**
- `src/kernel/fat32.h` / `fat32.c` - Added write operations (~350 lines)
- `src/kernel/io.h` - Added `inw()` / `outw()` functions
- `src/kernel.c` - VFS initialization, system directory creation
- `src/shell/shell.h` / `shell.c` - Updated prompt to show CWD
- `src/shell/commands.c` - New commands (cd, pwd, touch, rm) and VFS API usage

**Total Addition:** ~1000 lines of filesystem code

---

This implementation provides a complete, usable filesystem with directory navigation and file management suitable for an educational OS kernel.
