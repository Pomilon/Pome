# Language Fundamentals

This guide covers the basic building blocks of Pome: variables, data types, and expressions.

## Variables

### Declaration and Assignment

Use the `var` keyword to declare variables:

```pome
var message = "Hello";
var count = 42;
var pi = 3.14159;
```

Variables in Pome are **dynamically typed** - you don't specify a type when declaring them. The type is determined by the value assigned.

### Variable Naming

Variable names must follow these rules:

- Start with a letter (a-z, A-Z) or underscore (_)
- Contain only letters, digits (0-9), and underscores
- Are case-sensitive (`myVar` and `myvar` are different)

```pome
var myVar = 10;        // Valid
var _private = 20;     // Valid
var Count123 = 30;     // Valid
var 123invalid = 40;   // Invalid: starts with digit
var my-var = 50;       // Invalid: contains hyphen
```

### Reassignment

Variables can be reassigned to new values of any type:

```pome
var x = 10;
print(x);  // Output: 10

x = "now a string";
print(x);  // Output: now a string

x = nil;
print(x);  // Output: nil
```

## Data Types

Pome has the following primitive data types:

### Numbers

Pome treats all numeric values as the same type (no distinction between integers and floats):

```pome
var integer = 42;
var decimal = 3.14;
var negative = -100;
var scientific = 1.5e10;
```

### Strings

Strings are sequences of characters enclosed in double quotes:

```pome
var greeting = "Hello, World!";
var multiline = "Line 1
Line 2
Line 3";
```

**Escape sequences** are supported:

- `\"` - Double quote
- `\\` - Backslash
- `\n` - Newline
- `\t` - Tab
- `\r` - Carriage return

```pome
var escaped = "She said, \"Hello!\"";
print(escaped);  // Output: She said, "Hello!"
```

### Booleans

Boolean values represent truth:

```pome
var isActive = true;
var isInactive = false;
```

Booleans are commonly used in conditions:

```pome
if (true) {
    print("This always runs");
}

if (false) {
    print("This never runs");
}
```

### Nil

`nil` represents the absence of a value:

```pome
var nothing = nil;
var result;  // Implicitly nil
print(result);  // Output: nil
```

## Type Safety

Pome does **not** perform automatic type coercion. Operations between mismatched types (for example, adding a number and a string) will not be implicitly converted and may result in a runtime error or undefined behavior. Always ensure values are of the expected type before performing operations.

If you need to combine values of different kinds, convert them explicitly using appropriate utility functions or the standard library (for example, convert numbers to strings before concatenation, or parse strings to numbers before arithmetic).

## Comments

Use `//` for single-line comments:

```pome
var x = 10;  // This is a comment

// This entire line is a comment
var y = 20;
```

Multi-line comments are not currently supported in Pome.

## Expressions

### Arithmetic Expressions

Pome supports standard arithmetic operators:

```pome
var a = 10;
var b = 3;

print(a + b);      // Addition: 13
print(a - b);      // Subtraction: 7
print(a * b);      // Multiplication: 30
print(a / b);      // Division: 3.333...
print(a % b);      // Modulo: 1
print(a ^ b);      // Exponentiation: 1000
```

**Operator precedence** (from highest to lowest):

1. Unary (`-`, `!`)
2. Exponentiation (`^`)
3. Multiplication, Division, Modulo
4. Addition, Subtraction
5. Comparison
6. Logical AND
7. Logical OR
8. Assignment

### String Concatenation

Strings can be concatenated with the `+` operator:

```pome
var first = "Hello";
var second = "World";
print(first + " " + second);  // Output: Hello World
```

You can also concatenate numbers with strings (numbers are converted to strings):

```pome
var count = 5;
print("Count: " + count);  // Output: Count: 5
```

### Comparison Expressions

```pome
var x = 10;
var y = 20;

print(x == y);  // Equal: false
print(x != y);  // Not equal: true
print(x < y);   // Less than: true
print(x <= y);  // Less or equal: true
print(x > y);   // Greater than: false
print(x >= y);  // Greater or equal: false
```

### Logical Expressions

```pome
var a = true;
var b = false;

print(a and b);   // Logical AND: false
print(a or b);    // Logical OR: true
print(not a);     // Logical NOT: false
```

### Ternary Operator

The conditional operator provides a concise way to choose between two values:

```pome
var age = 25;
var status = age >= 18 ? "adult" : "minor";
print(status);  // Output: adult
```

Nested ternary operators:

```pome
var score = 85;
var grade = score >= 90 ? "A" : score >= 80 ? "B" : score >= 70 ? "C" : "F";
print(grade);  // Output: B
```

## Scope

Variables have **function scope** in Pome:

```pome
var global = "I am global";

fun testScope() {
    var local = "I am local";
    print(global);  // Prints: I am global
    print(local);   // Prints: I am local
}

testScope();
print(global);  // Prints: I am global
print(local);   // Error: undefined variable
```

## Type Introspection

### Getting Type Information

Use the `type()` function to get the type of a value:

```pome
print(type(42));           // Output: number
print(type(3.14));         // Output: number
print(type("hello"));      // Output: string
print(type(true));         // Output: boolean
print(type(nil));          // Output: nil
print(type([1, 2, 3]));    // Output: list
print(type({a: 1}));       // Output: table
```

### Truthiness

In boolean contexts, values are evaluated for truthiness:

```pome
if (0) { print("Zero is truthy"); }           // Does not print (0 is falsy)
if (1) { print("One is truthy"); }            // Prints
if ("") { print("Empty string is truthy"); }  // Does not print
if ("text") { print("Text is truthy"); }      // Prints
if (nil) { print("Nil is truthy"); }          // Does not print
if (false) { print("False is truthy"); }      // Does not print
```

Falsy values: `0`, `nil`, `false`  
Truthy values: Everything else (including empty strings)

## Constants

Pome does not have a built-in `const` keyword, but the convention is to use UPPER_CASE for values that shouldn't change:

```pome
var PI = 3.14159;
var MAX_ATTEMPTS = 3;
var DEFAULT_TIMEOUT = 5000;
```

## Best Practices

1. **Use descriptive names**: `user_age` is better than `a`
2. **Be consistent**: Use either `camelCase` or `snake_case` throughout your code
3. **Initialize variables**: Avoid relying on implicit `nil` values
4. **Keep scope minimal**: Declare variables as close to their usage as possible
5. **Avoid type juggling**: Explicitly convert types when needed for clarity

---

**Next**: [Control Flow](03-control-flow.md)  
**Back to**: [Documentation Home](README.md)
