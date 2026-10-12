# Operating Systems PA-02 Report

## Feature 1: Project Setup

- Created the required project directories.
- Added the starter source file `src/ls-v1.0.0.c`.
- Created a Makefile to compile the program.
- Built the program successfully using `make`.
- Tested the executable using `./bin/ls`.
- Committed and pushed the project to GitHub.

## Implemented Features

### 1. Long Listing (`-l`)
The `-l` option displays detailed information about each file, including file permissions, number of links, owner, group, file size, last modification time, and filename.

### 2. Alphabetical Sorting
Files and directories are sorted alphabetically by name before being displayed. Case-insensitive sorting is used to keep the listing organized.

### 3. Across-First Column Layout (`-x`)
The `-x` option displays filenames in a column layout, arranging entries across rows rather than showing each filename on a separate line.

### 4. Color Output (`--color`)
The `--color` option uses different colors to distinguish file types. Directories, symbolic links, and executable files are displayed in different colors to improve readability.

### 5. Recursive Listing (`-R`)
The `-R` option lists the contents of subdirectories recursively. It displays each subdirectory's path before showing its contents.

### 6. Compilation and Testing
The program is compiled using the provided Makefile and GCC with warning and debugging flags. The implemented options can be tested using commands such as `./bin/ls -l`, `./bin/ls -x`, `./bin/ls --color`, and `./bin/ls -R`.
