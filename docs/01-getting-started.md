# Getting Started with Pome

This guide will help you set up Pome and write your first program.

## Installation

### Prerequisites

Before installing Pome, ensure you have:

- **C++17 compatible compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake**: Version 3.10 or higher
- **Git**: For cloning the repository

### Building from Source

1. **Clone the repository**:
   ```bash
   git clone https://github.com/Pomilon/Pome.git
   cd Pome
   ```

2. **Create a build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**:
   ```bash
   cmake ..
   ```

4. **Build the project**:
   ```bash
   make
   ```

   On Windows with MSVC:
   ```bash
   cmake --build .
   ```

5. **Verify installation**:
   ```bash
   ./pome --version
   ```

## Running Your First Program

### Hello World

Create a file named `hello.pome`:

```pome
print("Hello, World!");
```

Run it:

```bash
./pome hello.pome
```

Expected output:
```
Hello, World!
```

### Interactive Mode

Pome supports reading from files:

```bash
./pome script.pome
```

### File Organization

Pome scripts should have the `.pome` file extension. Store your scripts anywhere convenient:

```
my-project/
├── main.pome
├── utils.pome
└── data/
    └── config.pome
```

## Common Tasks

### Running Multiple Files

You can only directly execute one file at a time, but you can import other modules:

```pome
import utils;
import config;

// Your code here
```

See [Module System](08-modules.md) for details on organizing code across files.

### Debugging Your Program

Add `print()` statements to debug:

```pome
var x = 10;
print("Debug: x =", x);
```

### Working with Examples

The repository includes examples in the `examples/` directory:

```bash
./pome ../examples/demo.pome
./pome ../examples/test_class.pome
./pome ../examples/test_stdlib.pome
```

## Troubleshooting

### CMake not found
Install CMake:
- **Ubuntu/Debian**: `sudo apt-get install cmake`
- **macOS**: `brew install cmake`
- **Windows**: Download from https://cmake.org/download/

### Compiler not found
Install a C++17 compiler:
- **Ubuntu/Debian**: `sudo apt-get install g++ cmake`
- **macOS**: `xcode-select --install`
- **Windows**: Visual Studio Community (includes MSVC)

### Build errors
1. Ensure you have C++17 or later
2. Try cleaning and rebuilding:
   ```bash
   cd build
   rm -rf *
   cmake ..
   make
   ```

### Script won't run
- Check that the file path is correct
- Verify the file has `.pome` extension
- Check for syntax errors (see [Language Fundamentals](02-language-fundamentals.md))

## Next Steps

1. Read [Language Fundamentals](02-language-fundamentals.md) to learn basic syntax
2. Explore [Control Flow](03-control-flow.md) for conditionals and loops
3. Check out [examples/](../examples/) for real-world patterns
4. Learn about [Functions](04-functions.md) and [OOP](05-oop.md)

---

**Back to**: [Documentation Home](README.md)
