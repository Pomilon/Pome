# Error Handling and Debugging

Learn how to write robust Pome code and debug issues.

## Common Errors

### Syntax Errors

Syntax errors occur when your code doesn't follow Pome's grammar.

#### Missing Semicolons

```pome
var x = 10   // Error: missing semicolon
var y = 20;
```

**Fix**: Add semicolons at the end of statements

```pome
var x = 10;
var y = 20;
```

#### Unmatched Braces

```pome
fun test() {
    print("missing closing brace");
// Error: expected '}'
```

**Fix**: Match all opening braces with closing ones

```pome
fun test() {
    print("fixed");
}
```

#### Incorrect Keywords

```pome
function add(a, b) {  // Error: 'function' is not valid
    return a + b;
}
```

**Fix**: Use correct keywords (e.g., `fun` instead of `function`)

```pome
fun add(a, b) {
    return a + b;
}
```

### Runtime Errors

Runtime errors occur when your code runs but encounters a problem.

#### Undefined Variables

```pome
print(unknownVariable);  // Error: undefined variable 'unknownVariable'
```

**Fix**: Declare the variable first

```pome
var unknownVariable = 10;
print(unknownVariable);
```

#### Accessing Undefined Properties

```pome
var person = {};
print(person.name);  // Returns nil (no error, but value is nil)
```

**Best practice**: Check for nil before using

```pome
var person = {};
if (person.name != nil) {
    print(person.name);
} else {
    print("Name not set");
}
```

#### Array Index Out of Bounds

```pome
var items = [1, 2, 3];
print(items[10]);  // Returns nil (out of bounds)
print(items[-1]);  // Returns 3 (negative index accesses from end)
print(items[-4]);  // Returns nil (out of bounds)
```

**Fix**: Check array length or ensure index is valid.

```pome
var items = [1, 2, 3];
var index = 10;
if (index >= 0 and index < len(items)) {
    print(items[index]);
} else {
    print("Index out of bounds");
}
```

#### Type Errors

```pome
var text = "hello";
print(text + 5);  // Output: hello5 (Pome performs implicit string conversion)
```

**Fix**: Ensure types match or convert explicitly

```pome
var text = "hello";
print(text + " 5");  // Concatenate with a string
```

#### Calling Non-Functions

```pome
var x = 42;
x();  // Error: tried to call a non-function
```

**Fix**: Only call variables that are functions

```pome
var fn = fun() { print("I'm a function"); };
fn();  // Correct
```

#### Division by Zero

```pome
var result = 10 / 0;  // May cause error or return infinity
```

**Fix**: Check for zero

```pome
var divisor = 0;
if (divisor != 0) {
    var result = 10 / divisor;
} else {
    print("Cannot divide by zero");
}
```

## Debugging Techniques

### Using print() for Debugging

Add print statements to trace execution:

```pome
fun calculateTotal(items) {
    print("DEBUG: calculateTotal called");
    var total = 0;
    
    for (var i = 0; i < len(items); i = i + 1) {
        var item = items[i];
        print("DEBUG: Processing item", i, ":", item);
        total = total + item;
    }
    
    print("DEBUG: Final total", total);
    return total;
}

var myItems = [1, 2, 3];
var result = calculateTotal(myItems);
```

Output:

```
DEBUG: calculateTotal called
DEBUG: Processing item 0 : 1
DEBUG: Processing item 1 : 2
DEBUG: Processing item 2 : 3
DEBUG: Final total 6
```

### Checking Variable Values

```pome
var x = 10;
var y = 20;
print("x =", x, ", y =", y);

var result = x + y;
print("result =", result);
```

### Inspecting Collections

```pome
var items = [1, 2, 3, 4, 5];
print("items =", items);
print("len(items) =", len(items));

var person = {name: "Alice", age: 30};
print("person =", person);
```

### Type Checking

```pome
var value = 42;
print("type(value) =", type(value));

var items = [1, 2, 3];
print("type(items) =", type(items));
```

## Defensive Programming

### Input Validation

```pome
fun divide(a, b) {
    if (type(a) != "number" or type(b) != "number") {
        print("Error: both arguments must be numbers");
        return nil;
    }
    
    if (b == 0) {
        print("Error: cannot divide by zero");
        return nil;
    }
    
    return a / b;
}

print(divide(10, 2));   // Output: 5
print(divide(10, 0));   // Error message, returns nil
print(divide("10", 2)); // Error message, returns nil
```

### Boundary Checking

```pome
fun getItem(list, index) {
    if (index < 0 or index >= len(list)) {
        print("Error: index out of bounds");
        return nil;
    }
    return list[index];
}

var items = ["a", "b", "c"];
print(getItem(items, 1));   // Output: b
print(getItem(items, 10));  // Error message
```

### Null/Nil Checking

```pome
fun processUser(user) {
    if (user == nil) {
        print("Error: user is nil");
        return;
    }
    
    if (user.name == nil) {
        print("Error: user.name is nil");
        return;
    }
    
    print("Processing user:", user.name);
}

var userEmpty = {};
var userAlice = {name: "Alice"};

processUser(nil);                    // Error message
processUser(userEmpty);              // Error message
processUser(userAlice);              // Processing user: Alice
```

## Error Prevention Patterns

### Default Values

```pome
fun greet(name) {
    if (name == nil) {
        name = "Guest";
    }
    print("Hello, " + name);
}

greet("Alice");  // Output: Hello, Alice
greet(nil);      // Output: Hello, Guest
```

### Safe Navigation

```pome
fun getUsername(user) {
    if (user == nil or user.profile == nil or user.profile.username == nil) {
        return "Unknown";
    }
    return user.profile.username;
}

var user1Profile = {username: "alice"};
var user1 = {profile: user1Profile};
var user2 = {profile: nil};
var user3 = nil;

print(getUsername(user1));  // Output: alice
print(getUsername(user2));  // Output: Unknown
print(getUsername(user3));  // Output: Unknown
```

### Graceful Degradation

```pome
fun loadConfiguration(filename) {
    import io; // Semicolon added
    
    var configContent = io.readFile(filename); // Semicolon added, use variable
    if (configContent == nil) {
        print("Warning: could not load", filename, ", using defaults"); // Semicolon added
        var defaultConfig = { // Use variable for table literal
            timeout: 5000,
            retries: 3
        }; // Semicolon added
        return defaultConfig; // Semicolon added
    }
    
    // Assuming configContent is parsed into a table somehow
    return configContent; // Semicolon added
}

var config = loadConfiguration("config.txt"); // Semicolon added
print("Timeout:", config.timeout); // Semicolon added
```

## Testing Your Code

### Unit Testing Pattern

```pome
fun assert(condition, message) {
    if (not condition) {
        print("ASSERTION FAILED:", message);
    } else {
        print("PASS:", message);
    }
}

fun add(a, b) {
    return a + b;
}

// Tests
assert(add(2, 3) == 5, "add(2, 3) should equal 5");
assert(add(-1, 1) == 0, "add(-1, 1) should equal 0");
assert(add(0, 0) == 0, "add(0, 0) should equal 0");
```

Output:

```
PASS: add(2, 3) should equal 5
PASS: add(-1, 1) should equal 0
PASS: add(0, 0) should equal 0
```

### Test Framework Pattern

```pome
fun runTests(testSuite) {
    var passed = 0;
    var failed = 0;
    
    for (var i = 0; i < len(testSuite); i = i + 1) {
        var test = testSuite[i];
        var name = test.name;
        var fn = test.fn;
        
        if (fn()) {
            print("✓", name);
            passed = passed + 1;
        } else {
            print("✗", name);
            failed = failed + 1;
        }
    }
    
    print(passed, "passed,", failed, "failed");
}

var tests = [
    {
        name: "string concatenation",
        fn: fun() {
            return "hello" + " " + "world" == "hello world";
        }
    },
    {
        name: "array length",
        fn: fun() {
            return len([1, 2, 3]) == 3;
        }
    }
];

runTests(tests);
```

## Tips for Robust Code

1. **Validate inputs early**: Check parameters before using them

   ```pome

var myList = [1, 2, 3]; // Semicolon added
fun process(data) {
    if (data == nil or type(data) != "list") {
        return nil; // Semicolon added
    }
    // ... process data
    return "Processed"; // Dummy return, semicolon added
}

var myList = [1, 2, 3];
print(process(nil));
print(process(myList));

   ```

2. **Use meaningful error messages**: Help users understand what went wrong
   ```pome
   if (age < 0) {
       print("Error: age cannot be negative");
       return nil;
   }
   ```

3. **Test edge cases**: Consider boundary conditions

   ```pome
   assert(add(0, 0) == 0, "zero + zero");
   assert(add(-1, 1) == 0, "negative plus positive");
   assert(add(999999, 1) == 1000000, "large numbers");
   ```

4. **Log important decisions**: Track control flow

   ```pome
   if (retry_count < MAX_RETRIES) {
       print("Retrying...");
       // retry logic
   } else {
       print("Max retries exceeded");
   }
   ```

5. **Isolate and test components**: Test functions independently

   ```pome
   fun testDoubleFunction() {
       assert(double(5) == 10, "double(5)");
       assert(double(0) == 0, "double(0)");
       assert(double(-5) == -10, "double(-5)");
   }
   
   testDoubleFunction();
   ```

---

**Next**: [Advanced Topics](11-advanced-topics.md)  
**Back to**: [Documentation Home](README.md)
