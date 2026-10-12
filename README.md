## Project Title
**Implementation of Linux `ls` Utility in C**

## Student Information
- **Name:** Fatima Nadeem
- **Roll Number:** BSDSF24M030
- **Course:** Operating Systems
- **Assignment:** PA-02

## Project Description
This project implements a simplified version of the Linux `ls` command using the C programming language. It demonstrates file-system operations, directory traversal, file metadata handling, command-line arguments, formatted output, sorting, and recursion.

The project is developed incrementally using Git feature branches, version tags, and GitHub releases.

## Features

- **Project Setup (v1.0.0):** Initial C program and Makefile.
- **Long Listing (v1.1.0):** The `-l` option displays detailed file information, including permissions, ownership, size, and modification time.
- **Column Display (v1.2.0):** Displays filenames in a down-then-across column layout.
- **Horizontal Display (v1.3.0):** The `-x` option arranges filenames across rows.
- **Alphabetical Sorting (v1.4.0):** Sorts filenames alphabetically.
- **Colorized Output (v1.5.0):** The `--color` option uses colors to distinguish file types.
- **Recursive Listing (v1.6.0):** The `-R` option lists subdirectory contents recursively.

## Project Structure

```text
BSDSF24M030-OS-A02/
├── src/
│   └── ls-v1.0.0.c
├── bin/
├── obj/
├── man/
├── Makefile
├── README.md
└── REPORT.md
```

## Requirements

- Ubuntu or another Linux distribution
- GCC compiler
- GNU Make
- Git

## Build Instructions

Clone the repository:

```bash
git clone https://github.com/FatimaNadeem7/BSDSF24M030-OS-A02.git
cd BSDSF24M030-OS-A02
```

Compile the program:

```bash
make
```

Run the program:

```bash
./bin/ls
```

## Usage Examples

```bash
./bin/ls
./bin/ls -l
./bin/ls -x
./bin/ls --color
./bin/ls -R
./bin/ls -lR
```

The supported options can be combined where appropriate.

## Testing

The program should be tested with directories containing regular files, subdirectories, executable files, symbolic links, and archive files. Compare the output with the expected behavior of the corresponding Linux `ls` options.

## Version Control

The project uses Git feature branches and version tags to track the development of each feature. Versioned releases are maintained through GitHub.

## Report

See [`REPORT.md`](REPORT.md) for the implementation analysis and answers to the assignment questions.

## Disclaimer

This is an educational implementation of selected `ls` features. It is not intended to reproduce every behavior of the standard GNU `ls` utility.
