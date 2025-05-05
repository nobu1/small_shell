# small_shell

A minimal Unix-like shell implemented in C.

## Overview

**small_shell** is a simple command-line shell, written in C, that mimics the basic functionality of popular Unix shells. It is designed for learning and experimentation, making it a great resource for those interested in operating systems, process management, and command parsing.

## Features

- Custom shell prompt
- Execution of external commands (searches `$PATH`)
- Built-in commands:
  - `cd`
  - `pwd`
  - `echo`
  - `export`
  - `unset`
  - `env`
  - `exit`
- Input/output redirection (`>`, `>>`, `<`)
- Pipes (`|`) for command chaining
- Environment variable expansion (e.g., `$HOME`)
- Basic error handling

## Getting Started

### Prerequisites

- GCC or compatible C compiler
- Unix-like OS (Linux, macOS, etc.)
- Make

### Build

Clone the repository and build:
```
git clone https://github.com/nobu1/small_shell.git  
cd small_shell  
gcc -o small_shell small_shell.c 
```

### Run
```./small_shell```

## Usage Examples
```
small_shell$ ls -l  
small_shell$ echo "Hello, World!"  
small_shell$ cd /tmp  
small_shell$ pwd  
small_shell$ cat file.txt | grep pattern > result.txt  
small_shell$ exit  
```

## Project Structure

- `small_shell.c` – Main shell implementation

## Author

[Shinji Nobuharta](https://github.com/nobu1)

---
