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

Pome supports short-circuit evaluation for `and` and `or` operators. The right-hand side is only evaluated if necessary.

```pome
var x = nil;

// This is safe because (x != nil) is false, so the second part is skipped.
if (x != nil and x.someProperty) {
    print("This won't crash");
}

// This is safe because (x == nil) is true, so the second part is skipped.
if (x == nil or x.someProperty) {
    print("Safe check");
}
```

## For-In Loops (ForEach)

The `for ... in` loop provides a convenient way to iterate over collections like lists and tables, or any object that implements the iterator protocol.

### Iterating Over Lists

When used with a list, the loop iterates over each element:

```pome
var fruits = ["apple", "banana", "cherry"];
for (var fruit in fruits) {
    print(fruit);
}
```

### Iterating Over Tables

When used with a table, the loop iterates over the **keys**:

```pome
var scores = {"Alice": 95, "Bob": 88};
for (var name in scores) {
    print(name, ":", scores[name]);
}
```

### Custom Iterators

You can make any class iterable by implementing an `iterator()` method that returns an object with a `next()` method.

```pome
class Counter {
    fun init(limit) {
        this.limit = limit;
        this.count = 0;
    }
    
    fun iterator() {
        return this;
    }
    
    fun next() {
        if (this.count < this.limit) {
            var val = this.count;
            this.count += 1;
            return val;
        }
        return nil; // Return nil to stop iteration
    }
}

var c = Counter(3);
for (var n in c) {
    print(n); // Output: 0, 1, 2
}
```

## Loop Control

### Break

The `break` statement exits the innermost loop immediately:

```pome
for (var i = 0; i < 10; i = i + 1) {
    if (i == 5) break;
    print(i);
}
// Output: 0, 1, 2, 3, 4
```

### Continue

The `continue` statement skips the rest of the current iteration and moves to the next one:

```pome
for (var i = 0; i < 5; i = i + 1) {
    if (i == 2) continue;
    print(i);
}
// Output: 0, 1, 3, 4
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
