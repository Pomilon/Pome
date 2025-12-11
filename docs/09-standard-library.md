# Standard Library Reference

Pome includes a standard library with built-in functions and modules.

## Global Functions

### print()

Output values to the console.

```pome
print("Hello");
print("Value:", 42);
print("Multiple", "values", "here");
```

Output:
```
Hello
Value: 42
Multiple values here
```

Multiple arguments are separated by spaces.

### len()

Get the length of a collection or string.

```pome
print(len([1, 2, 3]));        // Output: 3
print(len("hello"));           // Output: 5
print(len({a: 1, b: 2}));     // Output: 2
print(len(nil));               // May return 0 or error
```

Works with:
- Lists: Number of elements
- Strings: Number of characters
- Tables: Number of key-value pairs

### type()

Get the type of a value.

```pome
print(type(42));               // Output: number
print(type("text"));           // Output: string
print(type(true));             // Output: boolean
print(type(nil));              // Output: nil
print(type([1, 2]));           // Output: list
print(type({a: 1}));           // Output: table
print(type(fun() {}));         // Output: function
print(type(MyClass()));        // Output: instance
```

## Math Module

Mathematical functions and constants.

### Constants

```pome
import math;

print(math.pi);    // Output: 3.14159...
```

### Trigonometric Functions

```pome
import math;

print(math.sin(0));           // Output: 0
print(math.sin(math.pi / 2));  // Output: 1
print(math.cos(0));           // Output: 1
print(math.cos(math.pi));     // Output: -1
```

### Random Numbers

```pome
import math;

print(math.random());  // Random value between 0 and 1
var randInt = math.random() * 100;  // Random value between 0 and 100
```

## String Module

String manipulation functions.

### sub()

Extract a substring.

```pome
import string;

var text = "Hello World";
print(string.sub(text, 0, 5));    // Output: Hello
print(string.sub(text, 6, 11));   // Output: World
```

Parameters:
- `str`: The source string
- `start`: Starting index (0-based)
- `end`: Ending index (exclusive)

## IO Module

File input/output operations.

### readFile()

Read the contents of a file.

```pome
import io;

var content = io.readFile("myfile.txt");
print(content);
```

Returns: String containing the file contents

### writeFile()

Write content to a file.

```pome
import io;

var success = io.writeFile("output.txt", "Hello from Pome");
if (success) {
    print("File written successfully");
}
```

Parameters:
- `path`: File path to write to
- `content`: Content to write

Returns: Boolean indicating success

## Data Type Helpers

### Creating Collections

```pome
// Empty list
var empty = [];

// List with values
var numbers = [1, 2, 3];

// Empty table
var emptyTable = {};

// Table with values
var person = {name: "Alice", age: 30};
```

### Type Checking Patterns

```pome
fun processValue(val) {
    var t = type(val);
    
    if (t == "number") {
        print("Processing number:", val);
    } else if (t == "string") {
        print("Processing string:", val);
    } else if (t == "list") {
        print("Processing list with", len(val), "items");
    } else if (t == "table") {
        print("Processing table");
    }
}
```

## String Operations

While Pome doesn't have a full string library built-in, you can implement common operations:

### String Length

```pome
var text = "Hello";
print(len(text));  // Output: 5
```

### String Concatenation

```pome
var greeting = "Hello" + " " + "World";
print(greeting);  // Output: Hello World
```

### Substring

```pome
import string;

var text = "Hello World";
var part = string.sub(text, 0, 5);  // Output: Hello
```

## List Operations

### Creating Lists

```pome
var empty = [];
var numbers = [1, 2, 3, 4, 5];
var mixed = [1, "two", 3.0, true];
```

### Accessing Elements

```pome
var items = ["a", "b", "c"];
print(items[0]);   // Output: a
print(items[-1]);  // Output: c
```

### Length

```pome
var items = [1, 2, 3];
print(len(items));  // Output: 3
```

### Concatenation

```pome
var list1 = [1, 2];
var list2 = [3, 4];
var combined = list1 + list2;
print(combined);  // Output: [1, 2, 3, 4]
```

### Slicing

```pome
var numbers = [1, 2, 3, 4, 5];
print(numbers[1:3]);   // Output: [2, 3]
print(numbers[2:]);    // Output: [3, 4, 5]
```

## Table Operations

### Creating Tables

```pome
var empty = {};
var person = {name: "Bob", age: 25};
```

### Accessing Values

```pome
var person = {name: "Charlie", age: 30};
print(person.name);      // Output: Charlie
print(person["name"]);   // Output: Charlie
```

### Adding Keys

```pome
var config = {};
config.timeout = 5000;
config["retries"] = 3;
```

### Checking for Keys

```pome
var person = {name: "Diana"};
if (person.email == nil) {
    print("Email not set");
}
```

## Common Utility Patterns

### Implementing map()

```pome
fun map(list, fn) {
    var result = [];
    for (var i = 0; i < len(list); i = i + 1) {
        result = result + [fn(list[i])];
    }
    return result;
}

var numbers = [1, 2, 3];
var doubled = map(numbers, fun(x) { return x * 2; });
print(doubled);  // Output: [2, 4, 6]
```

### Implementing filter()

```pome
fun filter(list, predicate) {
    var result = [];
    for (var i = 0; i < len(list); i = i + 1) {
        if (predicate(list[i])) {
            result = result + [list[i]];
        }
    }
    return result;
}

var numbers = [1, 2, 3, 4, 5];
var evens = filter(numbers, fun(x) { return x % 2 == 0; });
print(evens);  // Output: [2, 4]
```

### Implementing reduce()

```pome
fun reduce(list, initial, fn) {
    var acc = initial;
    for (var i = 0; i < len(list); i = i + 1) {
        acc = fn(acc, list[i]);
    }
    return acc;
}

var numbers = [1, 2, 3, 4];
var sum = reduce(numbers, 0, fun(acc, x) { return acc + x; });
print(sum);  // Output: 10
```

## File I/O Examples

### Reading and Writing

```pome
import io;

// Write a file
io.writeFile("data.txt", "Hello, World!");

// Read the file
var content = io.readFile("data.txt");
print(content);  // Output: Hello, World!
```

### Processing File Content

```pome
import io;
import string;

var content = io.readFile("input.txt");
var length = len(content);
print("File is", length, "characters long");

// Extract first line
var firstLine = string.sub(content, 0, 10);
print("First 10 chars:", firstLine);
```

## Error Handling with Libraries

### Checking for Nil Results

```pome
import io;

var content = io.readFile("possibly_missing.txt");
if (content == nil) {
    print("File not found");
} else {
    print("File content:", content);
}
```

### Type Safety

```pome
fun safeSum(values) {
    if (type(values) != "list") {
        return nil;
    }
    
    var total = 0;
    for (var i = 0; i < len(values); i = i + 1) {
        var item = values[i];
        if (type(item) == "number") {
            total = total + item;
        }
    }
    return total;
}

print(safeSum([1, 2, 3]));      // Output: 6
print(safeSum([1, "two", 3]));  // Output: 4 (skips "two")
print(safeSum("not a list"));   // Output: nil
```

## Performance Considerations

### Efficient List Building

Avoid repeated concatenation in loops:

```pome
// Less efficient
var result = [];
for (var i = 0; i < 1000; i = i + 1) {
    result = result + [i];  // Creates new list each time
}

// Better approach with direct assignment
var result = [];
for (var i = 0; i < 1000; i = i + 1) {
    result[i] = i;  // Modifies in place
}
```

### String Building

```pome
// Multiple concatenations
var str = "a" + "b" + "c" + "d";

// More efficient single concatenation
var str = "a" + "b" + "c" + "d";
```

---

**Next**: [Error Handling](10-error-handling.md)  
**Back to**: [Documentation Home](README.md)
