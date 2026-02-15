# Pome Programming Language

<div align="center">
  <img src="assets/logo.png" alt="Pome Logo" width="150" height="150">
  <p><strong>A high-performance, register-based programming language written in C++</strong></p>
</div>

---

## Overview

**Pome** is a high-performance, modern programming language runtime. Originally created as a learning project to explore interpreter design, it has evolved into a production-ready system featuring a **Register-based Virtual Machine**, **NaN-boxing**, and a **Generational Garbage Collector**. 

Built entirely in C++17, Pome bridges the gap between the simplicity of dynamic scripting and the performance of low-level virtual machines. It is significantly faster than standard Python while maintaining a clean, expressive syntax inspired by Lua and JavaScript.

## 🚀 Performance (v0.2.0-beta)

Pome's modern architecture allows it to significantly outperform traditional AST-walking or stack-based interpreted languages.

| Runtime | 10M Loop Benchmark | Speedup |
| :--- | :--- | :--- |
| Python 3.12 | ~1.00s | 1.0x |
| **Pome v0.2** | **~0.24s** | **~4x Faster** |

*Benchmarks conducted on a compute-intensive loop (`while i < 10M`).*

## Key Features

### Core Language Features
- **Register-based VM**: Linear bytecode execution with register-to-register instructions, reducing instruction count and improving performance.
- **NaN-Boxing**: Efficient 64-bit value representation that minimizes memory footprint and maximizes CPU cache efficiency.
- **Object-Oriented Programming**: Full support for classes, inheritance, and optimized method dispatch.
- **Strict Mode**: Enforce safety with `strict pome;` to prevent accidental global pollution and undeclared variables.
- **Generational GC**: Modern garbage collection that optimizes for short-lived objects to reduce pause times.

### Developer Experience (DX)
- **LSP Support**: Built-in `pome-lsp` for real-time diagnostics and autocompletion in your editor.
- **Standard Formatter**: `pome-fmt` for consistent, "one-way" code styling.
- **Native Extensions (FFI)**: Stabilized C++ API for loading high-performance native modules (`.so` / `.dll`).

## Quick Start

### Installation (User-Local)

Pome can be installed quickly to your local user directory without requiring root privileges.

```bash
# Clone the repository
git clone https://github.com/pomilon/Pome.git
cd Pome

# Run the local installer
chmod +x install.sh
./install.sh
```

The installer builds Pome in **Release** mode and installs binaries to `~/.local/bin/` and resources to `~/.pome/`. Ensure `~/.local/bin` is in your `PATH`.

### Running Scripts

Pome scripts use the `.pome` extension:

```bash
pome script.pome
```

For the interactive shell (REPL):

```bash
pome
```

## Language Guide

### Variables and Data Types

```pome
strict pome; // Recommended for production

var x = 10;              // Number (Double)
var y = 3.14;            // Number
var message = "Hello";   // String
var flag = true;         // Boolean
var nothing = nil;       // Nil
var items = [1, 2, 3];   // List
var person = {           // Table (Associative Array)
    name: "Alice",
    age: 30
};
```

### Control Flow

```pome
// If-Else
if (x > 0) {
    print("Positive");
} else {
    print("Non-positive");
}

// Optimized While Loop
var counter = 0;
while (counter < 5) {
    print(counter);
    counter = counter + 1;
}

// For Loop
for (var i = 0; i < 3; i = i + 1) {
    print("Iteration:", i);
}
```

### Functions and Closures

```pome
fun add(a, b) {
    return a + b;
}

// First-class functions & Closures
fun makeCounter() {
    var count = 0;
    return fun() {
        count = count + 1;
        return count;
    };
}
```

### Object-Oriented Programming

```pome
class Dog {
    fun init(name) {
        this.name = name;
        this.sound = "Woof";
    }

    fun speak() {
        print(this.name, "says", this.sound);
    }
}

var dog = Dog("Buddy");
dog.speak(); // Output: Buddy says Woof
```

## Architecture

1. **Lexer**: Tokenizes source code into a stream of optimized tokens.
2. **Parser**: Builds an Abstract Syntax Tree (AST).
3. **Compiler**: Translates the AST into **Register-based Bytecode**.
4. **VM Engine**: Executes instructions using **Threaded Dispatch** (Computed GOTOs) for near-native performance.
5. **Memory**: A **Generational GC** with a **Write Barrier** ensures efficient memory management.
6. **Value System**: Uses **NaN-Boxing** to pack all types into 64 bits.

## Standard Library

- **`math`**: `sin`, `cos`, `random`, `pi`, `sqrt`.
- **`string`**: Substrings, length, concatenation.
- **`io`**: `readFile`, `writeFile` (Synchronous).
- **`time`**: `time.clock()` for high-precision timing.

## Project Structure

```
Pome/
├── install.sh              # User-local installer
├── include/                # VM and Compiler Headers
├── src/                    # VM and Compiler Implementation
├── benchmarks/             # Standard Performance Tests
├── test/                   # Functional Unit Tests
└── tools/                  # Test runners and comparative benchmarks
```

## Version

Current version: **0.2.0-beta**

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Happy Pome-ming!** 🍎✨
