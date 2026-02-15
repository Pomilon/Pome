# Pome Architecture and Implementation

This guide explains how Pome works internally. It's designed for contributors and those interested in language implementation.

## Overview

Pome follows a modern virtual machine architecture, split into a shared library (`libpome`) and a CLI executable (`pome`):

```
Source Code (.pome) / Native Module (.so/.dll)
    ↓
Lexer (Tokenization)
    ↓
Parser (AST Construction)
    ↓
Compiler (Bytecode Generation)
    ↓
Virtual Machine (Execution) ← Register-based
    ↓
Runtime Values & Memory
    ↓
Output
```

## Key Components

### 1. Lexer (`pome_lexer.cpp`)

**Purpose**: Convert source code into tokens.

The lexer reads the input string and breaks it into meaningful tokens. It now supports the `strict` keyword for enabling strict mode.

**Responsibilities**:
- Recognize keywords (`var`, `fun`, `if`, `class`, `strict`, etc.)
- Identify operators (`+`, `-`, `*`, `/`, `==`, etc.)
- Extract literals (numbers, strings)
- Handle comments (`//` and `/* */`)
- Track line/column information for errors

### 2. Parser (`pome_parser.cpp`)

**Purpose**: Build an Abstract Syntax Tree (AST) from tokens.

The parser takes the token stream and builds a hierarchical structure representing the program. It enforces syntax rules and handles operator precedence.

**Key Changes**:
- **Strict Mode Support**: Parses the `strict pome;` directive at the top of scripts.
- **LL(1) Recursive Descent**: Efficient single-token lookahead parsing.

### 3. Compiler (`pome_compiler.cpp`)

**Purpose**: Translate AST into VM Bytecode.

Unlike the prototype's AST walker, the modern Pome core compiles code into a linear array of instructions for a Register VM.

**Responsibilities**:
- **Register Allocation**: Maps local variables and temporaries to virtual registers (`R0`, `R1`, ...).
- **Instruction Emission**: Generates 32-bit opcodes (e.g., `ADD R1, R2, R3`).
- **Jump Patching**: Calculates offsets for control flow (`if`, `while`).
- **Static Analysis**: Enforces `strict` mode by checking for undeclared variable assignments at compile-time.

### 4. Virtual Machine (`pome_vm.cpp`)

**Purpose**: Execute Pome bytecode at high speed.

The VM is a **Register-based Machine** (similar to Lua 5.1), which reduces the number of instructions compared to stack-based machines.

**Features**:
- **Computed GOTOs**: Uses threaded dispatch (GCC/Clang labels) to eliminate `switch` statement overhead.
- **Fixed Register File**: Pre-allocated stack for fast register access without bounds checking in the hot loop.
- **Global Table**: A fast hash-map for global variable and native function lookups.

### 5. Value System (`pome_value.cpp`)

**Purpose**: Represent runtime values efficiently.

Pome uses **NaN-Boxing** to store all values in a single 64-bit `uint64_t`.

**Encoding**:
- **Doubles**: Standard IEEE 754 float representation.
- **Pointers**: Stored in the payload of a quiet NaN.
- **Singletons**: `nil`, `true`, and `false` have unique NaN payloads.

**Impact**: This halves memory usage per value and improves CPU cache performance by keeping data contiguous.

### 6. Generational Garbage Collector (`pome_gc.cpp`)

**Purpose**: Automatically manage memory with minimal pauses.

Pome implements a **Generational GC** based on the "Generational Hypothesis" (most objects die young).

**Architecture**:
- **Young Generation**: New objects are allocated here using a fast linked-list (or bump-pointer foundation).
- **Old Generation**: Surivors of GC cycles are promoted to the tenured heap.
- **Write Barrier**: Intercepts assignments to track Old-to-Young references, enabling efficient **Minor Collections** without scanning the entire heap.
- **VM Integration**: The GC correctly identifies roots on the VM stack and in the global table.

### 7. Module System (`pome_importer.cpp`)

**Purpose**: Load and manage module dependencies.

Handles both `.pome` scripts and **Native C++ Extensions**. Native extensions can be registered directly with the VM using the `registerNative` API.

---

## Performance Comparison

Pome's Register VM architecture is significantly faster than standard Python 3 for compute-intensive tasks.

| Implementation | 10M Loop Time | Relative Speed |
| :--- | :--- | :--- |
| Python 3.12 | ~1.00s | 1.0x |
| Pome v0.2 | ~0.22s | **4.5x Faster** |

## Soundness: Strict Mode

Enabled by adding `strict pome;` at the top of a file.

**Effects**:
- **Block Global Creation**: Assigning to a variable without `var` triggers a compiler error.
- **Declaration Enforcement**: All local variables must be declared before use.

---

**Back to**: [Documentation Home](README.md)