# Codepad

A lightweight Win32 syntax-highlighting text editor with integrated AES encryption.

## Features
- Native Win32 GDI rendering (fast, lightweight, no external runtime dependencies).
- Syntax highlighting and indentation guides.
- Multi-encoding support (ANSI, UTF-8, UTF-16, Win-1251, Win-1252).
- AES document encryption powered by Argon2 key derivation.
- Built-in "Find in Files" search pane.

## Building from Source

### Prerequisites
- Visual Studio (with C/C++ MSVC toolchain installed).

### Build Instructions
1. Open the **x64 Native Tools Command Prompt for VS**.
2. Run:
   ```cmd
   build-64.bat
   