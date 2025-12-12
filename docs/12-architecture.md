# Pome Architecture and Implementation

This guide explains how Pome works internally. It's designed for contributors and those interested in language implementation.

## Overview

Pome follows a classic interpreter architecture:

```
Source Code
    ↓
Lexer (Tokenization)
    ↓
Parser (AST Construction)
    ↓
Interpreter (Execution)
    ↓
Runtime Values & Environment
    ↓
Output
```

## Key Components

### 1. Lexer (`pome_lexer.cpp`)

**Purpose**: Convert source code into tokens

The lexer reads the input string and breaks it into meaningful tokens:

```
Input:  var x = 10 + 5;
Output: [VAR, IDENTIFIER("x"), ASSIGN, NUMBER(10), PLUS, NUMBER(5), SEMICOLON]
```

**Responsibilities**:

- Recognize keywords (`var`, `fun`, `if`, `class`, etc.)
- Identify operators (`+`, `-`, `*`, `/`, `==`, etc.)
- Extract literals (numbers, strings)
- Handle comments
- Track line/column information for errors

**Token Types** (from `pome_lexer.h`):

- Keywords: `FUNCTION`, `IF`, `ELSE`, `WHILE`, `FOR`, etc.
- Operators: `PLUS`, `MINUS`, `MULTIPLY`, `DIVIDE`, etc.
- Literals: `NUMBER`, `STRING`, `TRUE`, `FALSE`, `NIL`
- Delimiters: `LEFT_PAREN`, `RIGHT_PAREN`, `LEFT_BRACE`, etc.

### 2. Parser (`pome_parser.cpp`)

**Purpose**: Build an Abstract Syntax Tree (AST) from tokens

The parser takes the token stream and builds a hierarchical structure representing the program:

```
Tokens:     [VAR, IDENTIFIER("x"), ASSIGN, NUMBER(10), SEMICOLON]
AST:        VarStatement {
                name: "x",
                initializer: Literal { value: 10 }
            }
```

**Key Structures** (from `pome_ast.h`):

- **Statements**: Control flow and declarations
  - `VarStatement`: Variable declaration
  - `ExpressionStatement`: Expression evaluation
  - `IfStatement`: Conditional execution
  - `WhileStatement`: Loop with condition
  - `ForStatement`: C-style loop
  - `FunctionStatement`: Function definition
  - `ReturnStatement`: Return from function
  - `ClassStatement`: Class definition
  - `BlockStatement`: Grouped statements

- **Expressions**: Values and operations
  - `Literal`: Numbers, strings, booleans, nil
  - `BinaryExpression`: Operations with two operands
  - `UnaryExpression`: Operations with one operand
  - `TernaryExpression`: Conditional expression
  - `CallExpression`: Function/method call
  - `MemberExpression`: Object property access
  - `Identifier`: Variable reference
  - `ArrayExpression`: List literal
  - `ObjectExpression`: Table literal
  - `FunctionExpression`: Anonymous function
  - `ClassExpression`: Class reference

### 3. Interpreter (`pome_interpreter.cpp`)

**Purpose**: Execute the AST

The interpreter walks the AST and performs the described operations:

```
AST Node → Evaluate → Update Environment → Return Value
```

**Execution Model**:

- Traverses AST nodes recursively
- Maintains an environment for variable scope
- Calls appropriate evaluation methods for each node type
- Returns runtime values

**Key Methods**:

- `evaluate(statement)`: Execute a statement
- `evaluateExpression(expression)`: Compute an expression value
- `executeBlock(statements, environment)`: Run statements in a scope

### 4. Value System (`pome_value.cpp`)

**Purpose**: Represent runtime values

Pome values are represented by the `PomeValue` class hierarchy:

**Value Types** (from `pome_value.h`):

- **Primitives**:
  - Number: Double-precision floating point
  - String: UTF-8 encoded text
  - Boolean: True or false
  - Nil: Absence of value

- **Collections**:
  - List: Ordered array of values
  - Table: Key-value map

- **Functions**:
  - Function: User-defined function with closure
  - NativeFunction: Built-in C++ function

- **Object-Oriented**:
  - Class: Template for objects
  - Instance: Object created from a class

- **Modules**:
  - Module: Namespace of exported values

**Object Base Class**:

```cpp
class PomeObject {
public:
    virtual ObjectType type() const = 0;
    virtual std::string toString() const = 0;
    bool isMarked = false;  // For garbage collection
};
```

### 5. Environment (`pome_environment.cpp`)

**Purpose**: Manage variable scope and bindings

The environment maintains a mapping of variable names to values, with support for nested scopes:

```
Global Scope { x: 10, y: 20 }
    └─ Function Scope { z: 30 }
        └─ Block Scope { a: 40 }
```

**Key Operations**:

- `define(name, value)`: Create a new variable
- `get(name)`: Retrieve a variable's value
- `set(name, value)`: Update a variable's value
- `createChild()`: Create a nested scope (for functions, blocks, loops)

**Lexical Scoping**: Variables are looked up in the current scope, then parent scopes, up to global.

### 6. Garbage Collector (`pome_gc.cpp`)

**Purpose**: Automatically manage memory for objects

Pome uses a **mark-and-sweep garbage collector**:

**Algorithm**:

1. **Mark Phase**: Traverse all reachable objects from root references, marking them
2. **Sweep Phase**: Delete all unmarked objects

**Root References**:

- Global environment variables
- Local environment variables in call stack
- Object member variables
- Collection elements

**Triggering GC**:

- Runs periodically or when memory pressure is high
- Can be triggered manually (for debugging)

### 7. Module System (`pome_importer.cpp`)

**Purpose**: Load and manage module dependencies

**Process**:

1. Parse `import` statement to get module path
2. Load the `.pome` file
3. Execute the module code in its own environment
4. Collect exported definitions
5. Create a module namespace in the importing environment

**Module Environment**:

- Each module has its own scope
- Only exported items are accessible to importers
- Circular imports are prevented

### 8. Standard Library (`pome_stdlib.cpp`)

**Purpose**: Provide built-in functions and modules

**Built-in Functions**:

- `print()`: Output to console
- `len()`: Get collection length
- `type()`: Get value type

**Built-in Modules**:

- **math**: Math functions and constants
- **string**: String manipulation
- **io**: File I/O operations

## Execution Flow Example

Here's how Pome executes a simple program:

```pome
fun add(a, b) {
    return a + b;
}

var result = add(5, 3);
print(result);
```

### Step-by-Step Execution

1. **Lexer**:
   - Tokenizes the source code
   - Produces token stream

2. **Parser**:
   - Parses `fun add(a, b) { ... }` → FunctionStatement
   - Parses `var result = add(5, 3)` → VarStatement with CallExpression
   - Parses `print(result)` → ExpressionStatement with CallExpression

3. **Interpreter**:
   - Executes FunctionStatement: Defines `add` in environment
   - Executes VarStatement:
     - Evaluates CallExpression `add(5, 3)`
     - Creates new environment for function call
     - Binds `a = 5`, `b = 3`
     - Evaluates return statement: `5 + 3 = 8`
     - Assigns `result = 8`
   - Executes print statement: Looks up `result`, prints "8"

4. **Output**:

   ```
   8
   ```

## Type System

### Dynamic Typing

Pome uses **dynamic typing** - types are determined at runtime:

```cpp
PomeValue val;          // Uninitialized
val = PomeValue(42);    // Now a number
val = PomeValue("str"); // Now a string
```

### Type Coercion

Pome does **not** perform automatic type coercion. When operations involve incompatible types (for example, adding a number and a string), the interpreter does not implicitly convert values; such operations should be avoided or handled by explicit conversion in the program or standard library. This design keeps behavior predictable and avoids subtle bugs from implicit conversions.

## Operator Implementation

### Binary Operators

Handled by `evaluateBinaryExpression()` in the interpreter:

```cpp
if (op == "+") {
    // Concatenate if both operands are strings
    if (left.type == STRING && right.type == STRING) {
        return PomeValue(left.asString() + right.asString());
    }
    // Add if both operands are numbers
    if (left.type == NUMBER && right.type == NUMBER) {
        return PomeValue(left.asNumber() + right.asNumber());
    }
    // Otherwise, report a runtime type error (no implicit coercion)
    throw RuntimeError("Type error: unsupported operands for +");
}
```

### Operator Precedence

Implemented in the parser through recursive descent with different parsing levels for each precedence.

## Function Calls

### User-Defined Functions

```
1. Look up function name in environment
2. Create new environment with function's parent as parent
3. Bind parameters to arguments
4. Execute function body
5. Return result (or nil if no explicit return)
6. Restore previous environment
```

### Native Functions

```
1. Look up native function
2. Call C++ function with arguments
3. Return result
```

### Method Calls

```
1. Look up object
2. Look up method on object
3. Create new environment with 'this' bound to object
4. Execute method
```

## Class Instantiation

```pome
class Dog {
    fun init(name) {
        this.name = name;
    }
}

var dog = Dog("Buddy");
```

**Process**:

1. `Dog("Buddy")` calls the class as a function
2. Interpreter creates a new instance object
3. Looks up `init` method on the class
4. Calls `init` with `this` bound to the new instance
5. Returns the instance

## Closure Implementation

Closures are implemented by storing a reference to the defining environment:

```cpp
class PomeFunction : public PomeObject {
    // ...
    std::shared_ptr<Environment> closure;  // Captured environment
};
```

When the function is called, it creates a new environment with the closure as parent, preserving access to captured variables.

## Memory Management

### Reference Counting

The interpreter uses C++'s `std::shared_ptr` for automatic memory management:

```cpp
std::shared_ptr<PomeObject> obj = std::make_shared<PomeObject>();
// Automatically freed when no references remain
```

### Garbage Collection

Despite reference counting, a mark-and-sweep GC ensures circular references are handled:

```cpp
void markReachable(PomeValue value) {
    if (isObject(value) && !value.obj->isMarked) {
        value.obj->isMarked = true;
        markReferencesInObject(value.obj);
    }
}
```

## Parser Details

### Recursive Descent Parsing

Pome uses recursive descent parsing, a top-down parsing technique:

```cpp
Statement* parseStatement() {
    if (match(VAR)) return parseVarStatement();
    if (match(IF)) return parseIfStatement();
    if (match(WHILE)) return parseWhileStatement();
    // ...
}
```

### Operator Precedence Climbing

Binary operators are parsed using precedence climbing:

```cpp
Expression* parseBinary(int minPrecedence) {
    auto left = parsePrimary();
    while (precedence(peek()) >= minPrecedence) {
        auto op = advance();
        auto right = parseBinary(precedence(op) + (isRightAssociative(op) ? 0 : 1));
        left = createBinaryExpression(left, op, right);
    }
    return left;
}
```

(Note: Pome's implementation uses left-associativity for most binary operators, including `^` currently).

## Performance Considerations

### Interpretation Overhead

Pome is an interpreted language, so it's slower than compiled languages. For performance-critical code:

- Consider algorithmic improvements
- Minimize function calls in tight loops
- Use built-in functions (they're C++)

### Memory Usage

Garbage collection pauses can occur. For latency-sensitive applications:

- Be mindful of large data structures
- Avoid creating many short-lived objects

## Extending Pome

### Adding Native Functions

1. Define function in `pome_stdlib.cpp`:

```cpp
PomeValue nativeMyFunc(const std::vector<PomeValue>& args) {
    // Implementation
    return result;
}
```

2. Register in environment:

```cpp
env->define("myFunc", PomeValue(nativeMyFunc));
```

### Adding New Operators

1. Add token type to `pome_lexer.h`
2. Update lexer to recognize it
3. Update parser to handle it
4. Update interpreter to evaluate it

### Adding Built-in Modules

1. Create new module functions
2. Return a Table containing exported functions
3. Register in standard library

## Limitations and Future Work

### Current Limitations

- No inheritance for classes (composition pattern used instead)
- No break/continue statements in loops
- No exception handling (`try/catch`)
- No generator functions
- Limited standard library

### Potential Improvements

- Bytecode compilation for faster execution
- JIT compilation for hot paths
- Better error messages with stack traces
- More comprehensive standard library
- Pattern matching
- Async/await support

## Further Reading

- See `include/` directory for API details
- Review `examples/` for language usage patterns
- Check test files for edge cases and expected behavior

---

**Back to**: [Documentation Home](README.md)
