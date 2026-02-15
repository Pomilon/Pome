# Pome Programming Language

<div align="center">
  <img src="assets/logo.png" alt="Pome Logo" width="150" height="150">
  <p><strong>A high-performance, register-based programming language written in C++</strong></p>
</div>

---

## Overview

**Pome** is a high-performance, modern programming language runtime. Originally created as a learning project to explore interpreter design, it has evolved into a production-ready system featuring a **Register-based Virtual Machine**, **NaN-boxing**, and a **Generational Garbage Collector**. 

Built entirely in C++17, Pome bridges the gap between the simplicity of dynamic scripting and the performance of low-level virtual machines. It is significantly faster than standard Python while maintaining a clean, expressive syntax inspired by Lua and JavaScript.

> Note: Pome is supposed to be a successor of an old interpreted programming language I made. I improved the underlying architecture and extended its capabilities so I can potentially use it for actual programming tasks and to learn more about programming languages ecosystems.

## 🚀 Performance (v0.2.0-beta)

Pome's modern architecture allows it to significantly outperform traditional AST-walking or stack-based interpreted languages.

| Runtime | 10M Loop Benchmark | Speedup |
| :--- | :--- | :--- |
| Python 3.12 | ~1.00s | 1.0x |
| **Pome v0.2** | **~0.24s** | **~4x Faster** |

*Benchmarks conducted on a compute-intensive loop (`while i < 10M`).*

## Features

### Core Language Features

- **Register-based VM**: Linear bytecode execution with register-to-register instructions, reducing instruction count and improving performance.
- **NaN-Boxing**: Efficient 64-bit value representation that minimizes memory footprint and maximizes CPU cache efficiency.
- **Dynamic Typing**: No explicit type declarations required. Types are inferred at runtime.
- **Object-Oriented Programming**: Full support for classes, inheritance, and optimized method dispatch.
- **Functions**: First-class functions with closures and higher-order function support.
- **Control Flow**: Complete support for `if/else`, `while`, and `for` loops.
- **Operators**: Comprehensive operator support including arithmetic, comparison, logical, and assignment operators.
- **Strict Mode**: Enforce safety with `strict pome;` to prevent accidental global pollution and undeclared variables.

### Advanced Features

- **Generational GC**: Modern garbage collection that optimizes for short-lived objects to reduce pause times.
- **Native Extensions**: Extend Pome with high-performance C++ modules loaded dynamically (`.so`, `.dll`).
- **Module System**: Import and export modules for better code organization and reusability. Supports `import`, `from ... import`, and block exports `export { x, y }`.
- **Standard Library**: Built-in functions and modules:
  - **math**: Mathematical operations including `sin`, `cos`, `random`, and constants like `pi`.
  - **string**: String manipulation utilities like `sub` (substring).
  - **io**: File I/O operations (`readFile`, `writeFile`).
  - **time**: High-precision timing (`clock`, `sleep`).
  - **print**: Universal output function (supports multi-argument printing).

### Developer Experience (DX)
- **LSP Support**: Built-in `pome-lsp` for real-time diagnostics and autocompletion in your editor.
- **Standard Formatter**: `pome-fmt` for consistent, "one-way" code styling.

### Data Types

- **Primitives**: `nil`, `true`/`false`, numbers (integers and floats)
- **Collections**: Lists and Tables (associative arrays/dictionaries)
- **Complex Types**: Functions, Classes, and Instances

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

## Documentation

> Note: Documentation is slightly outdated and will be updated in the future.

For comprehensive guides on the Pome language, visit the **[docs/](docs/)** directory:

- **[Getting Started](docs/01-getting-started.md)** - Installation and first program
- **[Language Fundamentals](docs/02-language-fundamentals.md)** - Variables, types, and basic syntax
- **[Control Flow](docs/03-control-flow.md)** - Conditionals and loops
- **[Functions](docs/04-functions.md)** - Functions, closures, and higher-order functions
- **[Object-Oriented Programming](docs/05-oop.md)** - Classes and objects
- **[Collections](docs/06-collections.md)** - Lists and tables
- **[Operators Reference](docs/07-operators.md)** - Complete operator guide
- **[Module System](docs/08-modules.md)** - Code organization and imports
- **[Standard Library](docs/09-standard-library.md)** - Built-in functions and modules
- **[Error Handling](docs/10-error-handling.md)** - Debugging and testing
- **[Advanced Topics](docs/11-advanced-topics.md)** - Advanced patterns and techniques
- **[Architecture](docs/12-architecture.md)** - Internal design and implementation (for contributors)
- **[Native Extensions](docs/13-native-extensions.md)** - Writing C++ modules (FFI)

**[→ Full Documentation Index](docs/README.md)**

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

var counter = makeCounter();
print(counter());  // 1
print(counter());  // 2
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

### Module System

```pome
// Exporting from a module (in my_module.pome)
export fun add(a, b) {
    return a + b;
}

// Block export syntax
var x = 10;
var y = 20;
export { x, y };

// Importing and using a module
import my_module;
var result = my_module.add(5, 3);

// Using built-in modules
import math;
import string;
import io;

print("PI:", math.pi);
print("Substring:", string.sub("Hello", 0, 3));
io.writeFile("output.txt", "Hello, Pome!");
```

### Standard Library

#### Math Module

- `math.pi` - Pi constant
- `math.sin(x)` - Sine function
- `math.cos(x)` - Cosine function
- `math.random()` - Random number between 0 and 1

#### String Module

- `string.sub(str, start, end)` - Extract substring

#### IO Module

- `io.readFile(path)` - Read file contents
- `io.writeFile(path, content)` - Write content to file

#### Global Functions

- `print(...)` - Print values to stdout (supports multiple arguments)
- `len(collection)` - Get length of list or table
- `type(value)` - Get type name of value

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

## Architecture

1. **Lexer**: Tokenizes source code into a stream of optimized tokens.
2. **Parser**: Builds an Abstract Syntax Tree (AST).
3. **Compiler**: Translates the AST into **Register-based Bytecode**.
4. **VM Engine**: Executes instructions using **Threaded Dispatch** (Computed GOTOs) for near-native performance.
5. **Memory**: A **Generational GC** with a **Write Barrier** ensures efficient memory management.
6. **Value System**: Uses **NaN-Boxing** to pack all types into 64 bits.

## Examples

### Hello World

```pome
print("Hello, World!");
```

### Fibonacci Sequence

```pome
fun fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

for (var i = 0; i < 10; i = i + 1) {
    print("fib(" + i + ") = " + fibonacci(i));
}
```

### Working with Classes

```pome
class Calculator {
    fun init() {
        this.result = 0;
    }
    
    fun add(x) {
        this.result = this.result + x;
        return this;
    }
    
    fun multiply(x) {
        this.result = this.result * x;
        return this;
    }
    
    fun getResult() {
        return this.result;
    }
}

var calc = Calculator();
var answer = calc.add(5).multiply(3).getResult();
print("Result:", answer);  // Output: Result: 15
```

### File I/O

```pome
import io;

// Write to file
io.writeFile("greeting.txt", "Hello from Pome!");

// Read from file
var content = io.readFile("greeting.txt");
print("File contents:", content);
```

## Version

Current version: **0.2.0-beta**

## Platform Support

- **Linux**: Full support
- **macOS**: Supported (via CMake)
- **Windows**: Supported (via CMake/MSVC)

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## About This Project

Pome was created as a **learning project** to understand the fundamental concepts behind programming language implementation. Through building Pome, I explored:

- **Lexical Analysis**: Tokenizing source code
- **Syntax Analysis**: Building Abstract Syntax Trees
- **Semantic Analysis**: Type checking and scope management
- **Runtime Execution**: Interpreting and executing code (Virtual Machine)
- **Memory Management**: Implementing garbage collection (Generational Mark-and-Sweep)
- **Modularity**: Building an extensible module system

This refactored version improves upon the original implementation with better code organization and architecture.

### Inspiration

Pome draws inspiration from languages like Lua, Python, and Lox. Languages known for their clarity and educational value in language design.

---

**Happy Pome-ming!** 🍎✨