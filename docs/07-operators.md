# Operators Reference

A comprehensive guide to all operators in Pome, their usage, and precedence.

## Arithmetic Operators

Arithmetic operators perform mathematical calculations:

```pome
var a = 10;
var b = 3;

print(a + b);   // Addition: 13
print(a - b);   // Subtraction: 7
print(a * b);   // Multiplication: 30
print(a / b);   // Division: 3.333...
print(a % b);   // Modulo (remainder): 1
```

### Division

Division returns a decimal result:

```pome
print(10 / 3);    // Output: 3.333...
print(10 / 2);    // Output: 5
print(-10 / 3);   // Output: -3.333...
```

### Modulo

Modulo returns the remainder:

```pome
print(10 % 3);   // Output: 1
print(15 % 5);   // Output: 0
print(-10 % 3);  // Output: -1
```

## Comparison Operators

Comparison operators return boolean values:

```pome
print(5 == 5);   // Equal: true
print(5 == 3);   // Equal: false
print(5 != 3);   // Not equal: true
print(5 > 3);    // Greater than: true
print(5 < 3);    // Less than: false
print(5 >= 5);   // Greater or equal: true
print(3 <= 5);   // Less or equal: true
```

### Type Consideration

Comparison operators work with different types:

```pome
print(5 == "5");      // May be true (type coercion)
print(true == 1);     // Depends on implementation
print(nil == false);  // false
```

## Logical Operators

Logical operators work with boolean values:

```pome
var a = true;
var b = false;

print(a and b);   // Logical AND: false
print(a or b);    // Logical OR: true
print(not a);     // Logical NOT: false
```

### AND Operator

Returns true only if both operands are true:

```pome
print(true and true);    // true
print(true and false);   // false
print(false and true);   // false
print(false and false);  // false
```

Short-circuits - if left is false, right is not evaluated:

```pome
var x = nil;
if (x == nil and len(x) > 0) {
    // Second condition not evaluated because x == nil is true
}
```

### OR Operator

Returns true if at least one operand is true:

```pome
print(true or true);    // true
print(true or false);   // true
print(false or true);   // true
print(false or false);  // false
```

Short-circuits - if left is true, right is not evaluated:

```pome
if (x != nil or isValid(x)) {
    // If x != nil is true, isValid(x) is not called
}
```

### NOT Operator

Inverts a boolean value:

```pome
print(not true);   // false
print(not false);  // true
print(not nil);    // true (nil is falsy)
print(not 0);      // false (0 is truthy in Pome)
```

## String Operators

The `+` operator concatenates strings:

```pome
print("Hello" + " " + "World");  // Output: Hello World
print("Count: " + 5);            // Output: Count: 5
```

## Assignment Operator

The `=` operator assigns values to variables:

```pome
var x = 10;
x = 20;
x = x + 5;  // x is now 25
```

### Compound Assignment (Limited)

Pome doesn't have `+=`, `-=`, etc., but you can use:

```pome
var x = 10;
x = x + 5;   // instead of x += 5
x = x - 3;   // instead of x -= 3
```

## Operator Precedence

Operators are evaluated in this order (highest to lowest):

| Precedence | Operator | Associativity |
|-----------|----------|----------------|
| 1 | `()` `[]` `.` | Left to right |
| 2 | `!` `-` (unary) | Right to left |
| 3 | `*` `/` `%` | Left to right |
| 4 | `+` `-` | Left to right |
| 5 | `<` `<=` `>` `>=` | Left to right |
| 6 | `==` `!=` | Left to right |
| 7 | `and` | Left to right |
| 8 | `or` | Left to right |
| 9 | `?:` (ternary) | Right to left |
| 10 | `=` | Right to left |

### Examples

```pome
// Multiplication before addition
print(2 + 3 * 4);  // Output: 14 (not 20)

// Parentheses override precedence
print((2 + 3) * 4);  // Output: 20

// Comparison before AND
print(true and 5 > 3);  // Output: true

// AND before OR
print(true or false and false);  // Output: true (and is evaluated first)
print((true or false) and false);  // Output: false
```

## Unary Operators

### Unary Minus

Negates a number:

```pome
var x = 10;
print(-x);  // Output: -10
print(-(-x));  // Output: 10
```

### Logical NOT

Inverts a boolean:

```pome
print(!true);   // Output: false
print(!false);  // Output: true
```

## Ternary Operator

The conditional operator chooses between two values:

```pome
var age = 25;
var status = age >= 18 ? "adult" : "minor";
print(status);  // Output: adult
```

Syntax: `condition ? valueIfTrue : valueIfFalse`

### Nested Ternary

```pome
var score = 85;
var grade = score >= 90 ? "A" : 
            score >= 80 ? "B" : 
            score >= 70 ? "C" : 
            score >= 60 ? "D" : "F";
print(grade);  // Output: B
```

## Comparison with Different Types

### String vs Number

```pome
print("10" == 10);    // May vary
print("abc" == 123);  // false
```

### Nil Comparisons

```pome
print(nil == nil);      // true
print(nil == false);    // false
print(nil == 0);        // false
```

## Common Operator Patterns

### Checking Bounds

```pome
var value = 50;
if (value > 0 and value < 100) {
    print("In range");
}
```

### Checking Multiple Conditions

```pome
var x = 5;
if (x > 0 and x < 10 or x == 100) {
    print("Valid");
}
```

### Safe Property Access

```pome
var user = {};
if (user != nil and user.name != nil) {
    print(user.name);
}
```

### Toggle Booleans

```pome
var isActive = true;
isActive = not isActive;  // Now false
```

### Default Values

```pome
var input = nil;
var result = input != nil ? input : "default";
```

## Operator Tips

1. **Use parentheses for clarity**: Even when not required, they make code easier to read
   ```pome
   // Less clear
   if (x > 5 and y < 10 or z == 3) { }
   
   // Clearer
   if ((x > 5 and y < 10) or z == 3) { }
   ```

2. **Be aware of operator precedence**: Unexpected results come from forgetting it
   ```pome
   print(2 + 3 * 4);  // 14, not 20
   ```

3. **Use short-circuit evaluation strategically**:
   ```pome
   // This won't crash if x is nil
   if (x != nil and x > 10) { }
   ```

4. **Remember string coercion**:
   ```pome
   print("Value: " + myVar);  // Converts myVar to string
   ```

---

**Next**: [Module System](08-modules.md)  
**Back to**: [Documentation Home](README.md)
