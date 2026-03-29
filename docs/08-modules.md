# Module System

Organize your code into modules for better reusability and maintainability.

## What are Modules?

Modules are separate files containing Pome code that you can import into other files. They allow you to:

- Organize code into logical units
- Reuse code across multiple projects
- Hide implementation details
- Manage dependencies

## File Structure

Modules are `.pome` files:

```
project/
├── main.pome
├── math_utils.pome
├── string_utils.pome
└── data/
    └── config.pome
```

## Exporting Functions and Values

Use the `export` keyword to make code available to other modules:

### Exporting Single Declarations

```pome
// math_utils.pome
export fun add(a, b) {
    return a + b;
}

export var pi = 3.14159;
```

### Exporting Multiple Symbols (Export Blocks)

You can export multiple items at once using an export block:

```pome
var a = 1, b = 2;
fun sum(x, y) { return x + y; }

export { a, b, sum };
```

## Importing Modules

### Standard Import

The `import` statement loads an entire module:

```pome
import math_utils;

print(math_utils.add(5, 3));
```

### Selected Import (From-Import)

Use `from ... import` to pull specific symbols into your local scope:

```pome
from math_utils import add, pi;

print(add(5, 3));
print(pi);
```

## Module Search Paths

When you import a module, Pome looks for it in the following order:

1. **Script Directory**: The directory containing the script being executed.
2. **Current Directory**: The current working directory.
3. **Environment Variables**: Directories listed in `POME_PATH`.
4. **Built-in Modules**: Modules like `math`, `io`, `string`, `time`.

### Path-Based Imports

You can import modules in subdirectories using path separators:

```pome
import utils/string_helper;

// Access symbols via the base name:
string_helper.trim(" text ");
```

Pome automatically uses the last part of the path as the module's name in your local scope.

## Built-in Modules

Pome includes several built-in modules:

### Math Module

```pome
import math;

print(math.pi);
print(math.sin(math.pi / 2));
print(math.cos(0));
print(math.random());
```

### String Module

```pome
import string;

var text = "Hello World";
var substring = string.sub(text, 0, 5);
print(substring);  // Output: Hello
```

### IO Module

```pome
import io;

io.writeFile("output.txt", "Hello from Pome");
var content = io.readFile("output.txt");
print(content);
```

## Module Best Practices

1. **One responsibility per module**: Keep related functions together

   ```pome
   // Good structure
   math_utils.pome    // Math operations
   string_utils.pome  // String operations
   models.pome        // Data models
   ```

2. **Use clear names**: Module names should describe their content

   ```pome
   user_validator.pome      // Validates users
   database_connection.pome // Database operations
   ```

3. **Hide implementation details**: Only export necessary items

   ```pome
   // string_utils.pome
   fun _private_helper(str) {
       // Internal implementation
       return str;
   }
   
   export fun publicFunction(str) {
       return _private_helper(str);
   }
   ```

4. **Document exported interfaces**: Make clear what modules provide

   ```pome
   // config.pome
   // Configuration module provides application settings
   // Exports: DB_HOST, DB_PORT, LOG_LEVEL, getConfig()
   
   export var DB_HOST = "localhost";
   export var DB_PORT = 5432;
   ```

5. **Avoid circular dependencies**: Module A importing Module B that imports A

   ```pome
   // Avoid this structure
   // models/user.pome imports services/user_service.pome
   // services/user_service.pome imports models/user.pome
   ```

6. **Group related modules in directories**:

   ```
   project/
   ├── models/
   │   ├── user.pome
   │   ├── book.pome
   │   └── comment.pome
   ├── services/
   │   ├── user_service.pome
   │   ├── book_service.pome
   │   └── ...
   └── utils/
       ├── math_utils.pome
       └── string_utils.pome
   ```

## Example Project Structure

```
my_app/
├── main.pome
├── config.pome
├── models/
│   ├── user.pome
│   ├── product.pome
│   └── order.pome
├── services/
│   ├── user_service.pome
│   ├── product_service.pome
│   └── order_service.pome
├── utils/
│   ├── validation.pome
│   ├── formatting.pome
│   └── array_utils.pome
└── data/
    └── database.pome
```

```pome
// main.pome
import config;
import models/user;
import models/product;
import services/user_service;
import utils/validation;

// Application code
```

---

**Next**: [Standard Library](09-standard-library.md)  
**Back to**: [Documentation Home](README.md)
