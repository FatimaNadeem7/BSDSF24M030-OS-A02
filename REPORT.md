# Operating Systems PA-02 Report

## Feature 1: Project Setup

- Created the required project directories: `src/`, `bin/`, `obj/`, and `man/`.
- Added the source file `src/ls-v1.0.0.c`.
- Created a Makefile to compile the program using GCC.
- Built the program using `make`.
- Tested the executable using `./bin/ls`.
- Used Git branches, commits, and tags to manage development.
- Pushed the project to GitHub.

## Feature 2: Long Listing (`-l`)

The `-l` option displays detailed information about files, including file type, permissions, number of hard links, owner, group, size, modification time, and filename.

### Use of `stat()` and `lstat()`

The `lstat()` system call obtains information about a file without following a symbolic link. This allows the program to identify symbolic links themselves. The program uses file metadata to determine file types, permissions, ownership, file size, and modification time.

### File Permissions

File permissions are stored in the file's mode bits. The program uses bitwise AND operations with constants such as `S_IRUSR`, `S_IWUSR`, and `S_IXUSR` to determine whether the owner, group, and others have read, write, and execute permissions.

### Implementation Decision

The `-l` option provides a detailed, human-readable listing. User and group IDs are converted to names using `getpwuid()` and `getgrgid()`. Symbolic links are displayed with their targets when available.

## Feature 3: Default Column Display

The program displays filenames in columns when the `-l` option is not used. It calculates the maximum filename length and uses the terminal width to determine how many columns fit on a line.

The default layout fills entries down each column before moving to the next column.

## Feature 4: Horizontal Display (`-x`)

The `-x` option displays filenames across each row from left to right before moving to the next row.

This differs from the default column layout, which fills entries down columns first. The same filename sorting is retained in both layouts.

## Feature 5: Alphabetical Sorting

The program sorts filenames alphabetically before displaying them. It uses `qsort()` with a comparison function based on `strcasecmp()`, with `strcmp()` as a tie-breaker.

The sorting algorithm used internally by `qsort()` is implementation-dependent. Its typical time complexity is O(n log n), although the C standard does not guarantee a specific complexity.

Sorting is performed separately for each directory.

## Feature 6: Colorized Output (`--color`)

The `--color` option uses ANSI terminal escape sequences to distinguish file types.

The required color scheme is:

- **Blue:** Directories
- **Green:** Executable files
- **Red:** Archive files, including `.tar`, `.gz`, and `.zip`
- **Pink/Magenta:** Symbolic links
- **Reverse video:** Special files, such as character devices, block devices, FIFOs, and sockets

The program uses file metadata to identify directories, symbolic links, executables, and special files. Archive files are identified by their filename extensions.

## Feature 7: Recursive Listing (`-R`)

The `-R` option lists the contents of a directory and its subdirectories recursively.

The program uses `opendir()` and `readdir()` to read directory entries, and `lstat()` to identify subdirectories without following symbolic links. Each discovered subdirectory is processed recursively.

The program excludes `.` and `..` to prevent immediate traversal back to the current or parent directory.

## Compilation and Testing

The program is compiled with GCC using the supplied Makefile and the warning options `-Wall -Wextra -g`.

Build commands:

```bash
make clean
make
```

Test commands:

```bash
./bin/ls
./bin/ls -l
./bin/ls -x
./bin/ls --color
./bin/ls -R
./bin/ls -lR
./bin/ls -x --color
```

These tests cover ordinary listing, detailed metadata, horizontal layout, colorized output, recursive traversal, and combined options.

## Conclusion

This project implements a simplified `ls` utility in C. It demonstrates directory traversal, file metadata retrieval, permission-bit inspection, alphabetical sorting, column formatting, ANSI color output, and recursive directory listing. Git branches, commits, and version tags are used to manage the project's development history.
