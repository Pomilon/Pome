# Control Flow

Control flow statements allow you to decide which code runs and how many times it runs.

## If Statements

The `if` statement runs code only if a condition is true:

```pome
if (true) {
    print("This always runs");
}

if (false) {
    print("This never runs");
}
```

### Conditions

The condition can be any expression that evaluates to a boolean:

```pome
var age = 25;
if (age >= 18) {
    print("You are an adult");
}

var name = "Alice";
if (name == "Alice") {
    print("Hello, Alice!");
}

var x = 10;
if (x > 5 and x < 15) {
    print("x is between 5 and 15");
}
```

### If-Else

Run different code based on a condition:

```pome
var score = 75;

if (score >= 90) {
    print("A");
} else {
    print("B or lower");
}
```

### If-Else If-Else

Chain multiple conditions:

```pome
var score = 75;

if (score >= 90) {
    print("Grade: A");
} else if (score >= 80) {
    print("Grade: B");
} else if (score >= 70) {
    print("Grade: C");
} else if (score >= 60) {
    print("Grade: D");
} else {
    print("Grade: F");
}
```

## While Loops

The `while` loop repeats code as long as a condition is true:

```pome
var count = 0;
while (count < 5) {
    print("Count:", count);
    count = count + 1;
}
```

Output:
```
Count: 0
Count: 1
Count: 2
Count: 3
Count: 4
```

### Loop Control

Be careful not to create infinite loops:

```pome
// This will run forever!
// var x = 1;
// while (x > 0) {
//     print(x);
// }
```

### Using While for Complex Logic

```pome
var total = 0;
var i = 1;
while (i <= 10) {
    total = total + i;
    i = i + 1;
}
print("Sum from 1 to 10:", total);  // Output: Sum from 1 to 10: 55
```

## For Loops

The `for` loop provides a compact way to repeat code a fixed number of times:

```pome
for (var i = 0; i < 5; i = i + 1) {
    print("i:", i);
}
```

Output:
```
i: 0
i: 1
i: 2
i: 3
i: 4
```

### For Loop Anatomy

A `for` loop has three parts:

```pome
for (initialization; condition; increment) {
    // Loop body
}
```

- **Initialization**: Runs once before the loop starts
- **Condition**: Checked before each iteration (loop stops when false)
- **Increment**: Runs after each iteration

### For Loop Examples

Countdown:
```pome
for (var i = 10; i > 0; i = i - 1) {
    print(i);
}
print("Blastoff!");
```

Multiplication table:
```pome
for (var i = 1; i <= 5; i = i + 1) {
    for (var j = 1; j <= 5; j = j + 1) {
        print(i * j, "  ");
    }
    print("");
}
```

### Accessing Loop Variables

Variables declared in the loop initialization are scoped to the loop:

```pome
for (var i = 0; i < 5; i = i + 1) {
    print(i);
}
print(i);  // Error: i is not defined outside the loop
```

To use the variable after the loop, declare it outside:

```pome
var i;
for (i = 0; i < 5; i = i + 1) {
    print(i);
}
print("Final value of i:", i);  // Output: Final value of i: 5
```

## Ternary Operator

The ternary operator is a compact way to choose between two values:

```pome
var age = 25;
var status = age >= 18 ? "adult" : "minor";
print(status);  // Output: adult
```

This is equivalent to:

```pome
var age = 25;
var status;
if (age >= 18) {
    status = "adult";
} else {
    status = "minor";
}
print(status);
```

### Nested Ternary

```pome
var score = 85;
var grade = score >= 90 ? "A" : score >= 80 ? "B" : score >= 70 ? "C" : "F";
print(grade);  // Output: B
```

**Note**: Deeply nested ternary operators can be hard to read. Consider using `if-else` for clarity.

## Logical Operators

Logical operators help build complex conditions:

### AND Operator

`and` returns true only if both sides are true:

```pome
var age = 25;
var hasLicense = true;

if (age >= 18 and hasLicense) {
    print("You can drive");
}
```

### OR Operator

`or` returns true if at least one side is true:

```pome
var day = "Saturday";

if (day == "Saturday" or day == "Sunday") {
    print("It's the weekend!");
}
```

### NOT Operator

`not` inverts a boolean value:

```pome
var isRaining = false;

if (not isRaining) {
    print("You don't need an umbrella");
}
```

### Short-Circuit Evaluation

Pome uses short-circuit evaluation:

```pome
var x = nil;

// Second part never evaluated because x == nil is false
if (x == nil or x > 10) {
    print("x is nil or greater than 10");
}

// Second part never evaluated because first is true
if (x != nil or true) {
    print("This always prints");
}
```

## Breaking Out of Loops

Pome does not currently support `break` or `continue` statements. To exit a loop early, use a conditional:

```pome
var found = false;
for (var i = 0; i < 10; i = i + 1) {
    if (i == 5) {
        found = true;
    }
    if (not found) {
        print(i);
    }
}
```

## Best Practices

1. **Keep conditions simple**: Break complex conditions into multiple lines
   ```pome
   if (age >= 18 and 
       hasLicense and 
       not isDisqualified) {
       print("Can drive");
   }
   ```

2. **Avoid deeply nested conditions**: Consider extracting functions
   ```pome
   fun canDrive(age, hasLicense) {
       return age >= 18 and hasLicense;
   }
   
   if (canDrive(age, hasLicense)) {
       print("Can drive");
   }
   ```

3. **Use meaningful loop variables**: `i` is fine for simple counters, but use descriptive names otherwise
   ```pome
   for (var attempt = 1; attempt <= maxAttempts; attempt = attempt + 1) {
       // ...
   }
   ```

4. **Avoid infinite loops**: Always ensure loop conditions will eventually become false

---

**Next**: [Functions](04-functions.md)  
**Back to**: [Documentation Home](README.md)
