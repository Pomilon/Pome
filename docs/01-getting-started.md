# Getting Started with Pome

This guide will help you set up Pome and write your first program.

## Installation

### Prerequisites

Before installing Pome, ensure you have:

- **C++17 compatible compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake**: Version 3.10 or higher
- **Git**: For cloning the repository

### Building and Installing (User-Local)

Pome 0.2 provides a simplified installation script that builds the project and installs it to your local user directory (`~/.pome` and `~/.local/bin`).

1. **Clone the repository**:

   ```bash
   git clone https://github.com/Pomilon/Pome.git
   cd Pome
   ```

2. **Run the installer**:

   ```bash
   chmod +x install.sh
   ./install.sh
   ```

3. **Verify installation**:

   Ensure `~/.local/bin` is in your `PATH`, then run:

   ```bash
   pome --version
   ```

## Running Your First Program

### The Interactive Shell (REPL)

Simply run `pome` without arguments to enter the interactive shell:

```bash
pome
```

You will see the Pome logo and system information. You can type Pome code directly:

```pome
pome> print("Hello from the VM!");
Hello from the VM!
```

### Running a Script

Create a file named `hello.pome`:

```pome
strict pome; // Optional: Enable strict mode
print("Hello, World!");
```

Run it:

```bash
pome hello.pome
```

Expected output:

```
Hello, World!
```

## Next Steps

1. Read [Language Fundamentals](02-language-fundamentals.md) to learn basic syntax.
2. Explore [Control Flow](03-control-flow.md) for conditionals and loops.
3. Check out the [Architecture](12-architecture.md) guide to learn about the Register VM and NaN-Boxing.
4. Learn about [Functions](04-functions.md) and [OOP](05-oop.md).

---

**Back to**: [Documentation Home](README.md)