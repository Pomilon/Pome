# Functions

Functions are reusable blocks of code that perform specific tasks.

## Defining Functions

Use the `fun` keyword to define a function:

```pome
fun greet(name) {
    print("Hello, " + name);
}
```

### Function Components

```pome
fun functionName(parameter1, parameter2) {
    // Function body
    print(parameter1, parameter2);
}
```

- **Function name**: Used to call the function later
- **Parameters**: Values passed to the function (optional)
- **Function body**: Code that runs when the function is called

## Calling Functions

Call a function using its name with parentheses:

```pome
fun greet(name) {
    print("Hello, " + name);
}

greet("Alice");   // Output: Hello, Alice
greet("Bob");     // Output: Hello, Bob
```

## Parameters and Arguments

Functions can accept multiple parameters:

```pome
fun add(a, b) {
    print(a + b);
}

add(5, 3);  // Output: 8
add(10, 20);  // Output: 30
```

Parameters are local to the function:

```pome
fun test(x) {
    x = 100;
    print("Inside function:", x);
}

var x = 50;
test(x);
print("Outside function:", x);  // x is still 50
```

Output:

```
Inside function: 100
Outside function: 50
```

## Return Values

Functions can return values using the `return` keyword:

```pome
fun add(a, b) {
    return a + b;
}

var result = add(5, 3);
print(result);  // Output: 8
```

### Early Return

Return can appear multiple times in a function:

```pome
fun max(a, b) {
    if (a > b) {
        return a;
    }
    return b;
}

print(max(10, 20));  // Output: 20
```

### Functions Without Return

If a function doesn't explicitly return a value, it returns `nil`:

```pome
fun noReturn() {
    print("This function has no return");
}

var result = noReturn();
print(result);  // Output: nil
```

## First-Class Functions

In Pome, functions are **first-class values**. You can assign them to variables:

```pome
fun add(a, b) {
    return a + b;
}

var operation = add;
print(operation(5, 3));  // Output: 8
```

## Anonymous Functions

Pome supports anonymous function expressions, also known as lambdas. These functions don't have a name and can be assigned to variables or passed as arguments.

```pome
var multiply = fun(a, b) {
    return a * b;
};

print(multiply(4, 5));  // Output: 20
```

### Anonymous Functions in Variables

You can assign function literals directly to variables:

```pome
var square = fun(x) {
    return x * x;
};

print(square(5));  // Output: 25
```

## Closures

Functions in Pome form **closures**, meaning they capture and retain access to variables from their defining scope, even after the outer function has returned.

```pome
fun makeAdder(amount) {
    return fun(x) {
        return x + amount;
    };
}

var addFive = makeAdder(5);
print(addFive(10));  // Output: 15

var addTen = makeAdder(10);
print(addTen(10));   // Output: 20
```

### Practical Closure Example

Closures are useful for maintaining private state:

```pome
fun makeCounter() {
    var count = 0;
    return fun() {
        count = count + 1;
        return count;
    };
}

var counter = makeCounter();
print(counter());  // Output: 1
print(counter());  // Output: 2
print(counter());  // Output: 3
```

Each call to `makeCounter()` creates a new `count` variable with its own closure.

## Higher-Order Functions

Functions that take other functions as parameters or return them are called higher-order functions.

```pome
fun apply(operation, a, b) {
    return operation(a, b);
}

fun add(a, b) {
    return a + b;
}

fun multiply(a, b) {
    return a * b;
}

print(apply(add, 5, 3));       // Output: 8
print(apply(multiply, 5, 3));  // Output: 15
```

### Using Higher-Order Functions for Callbacks

You can pass anonymous functions directly as callbacks:

```pome
fun repeatTimes(count, callback) {
    for (var i = 0; i < count; i = i + 1) {
        callback(i);
    }
}

repeatTimes(3, fun(i) {
    print("Iteration", i);
});
```

Output:

```
Iteration 0
Iteration 1
Iteration 2
```

## Scope and Functions

Variables declared inside a function are local to that function:

```pome
fun test() {
    var local = "I'm local";
}

test();
print(local);  // Error: undefined variable 'local'
```

Functions can access variables from outer scopes (lexical scoping):

```pome
var global = "I'm global";

fun test() {
    print(global);  // Can access global
}

test();  // Output: I'm global
```

## Default Parameters

Pome doesn't support default parameters directly, but you can work around it:

```pome
fun greet(name) {
    var actualName = name;
    if (actualName == nil) {
        actualName = "Guest";
    }
    print("Hello, " + actualName);
}

greet("Alice");  // Output: Hello, Alice
greet(nil);      // Output: Hello, Guest
greet();         // Error: expected 1 argument, got 0
```

## Variadic Functions

Pome doesn't have built-in variadic function support, but you can pass lists:

```pome
fun sum(numbers) {
    var total = 0;
    for (var i = 0; i < len(numbers); i = i + 1) {
        total = total + numbers[i];
    }
    return total;
}

print(sum([1, 2, 3, 4, 5]));  // Output: 15
```

## Recursion

Functions can call themselves:

```pome
fun factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

print(factorial(5));  // Output: 120
```

### Tail Recursion

Pome doesn't optimize tail calls, so deep recursion may cause issues:

```pome
fun countdown(n) {
    if (n <= 0) {
        print("Blastoff!");
        return;
    }
    print(n);
    countdown(n - 1);
}

countdown(3);
```

Output:

```
3
2
1
Blastoff!
```

## Best Practices

1. **Use meaningful names**: `calculateTotal` is better than `calc`

2. **Keep functions focused**: A function should do one thing well

   ```pome
   // Good
   fun getUsername(user) {
       return user.name;
   }
   
   fun formatGreeting(name) {
       return "Hello, " + name;
   }
   
   // Less good
   fun processUser(user) {
       var name = user.name;
       var greeting = "Hello, " + name;
       print(greeting);
       return name;
   }
   ```

3. **Document complex logic**: Use comments for non-obvious code

   ```pome
   // Fibonacci using dynamic programming (avoiding deep recursion)
   fun fib(n) {
       if (n <= 1) return n;
       // ... implementation
   }
   ```

4. **Use closures wisely**: They're powerful but can be hard to follow

   ```pome
   // Clear closure usage
   var operations = {
       add: fun(a, b) { return a + b; },
       subtract: fun(a, b) { return a - b; }
   };
   ```

5. **Avoid deeply nested functions**: Can impact readability

   ```pome
   // Prefer
   fun outer() {
       var innerFunc = fun() {
           return 42;
       };
       return innerFunc();
   }
   
   // Over
   fun outer() {
       return (fun() {
           return (fun() {
               return 42;
           })();
       })();
   }
   ```

---

**Next**: [Object-Oriented Programming](05-oop.md)  
**Back to**: [Documentation Home](README.md)
