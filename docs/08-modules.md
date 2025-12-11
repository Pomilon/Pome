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

### Exporting Functions

```pome
// math_utils.pome
export fun add(a, b) {
    return a + b;
}

export fun multiply(a, b) {
    return a * b;
}
```

### Exporting Classes

```pome
// models.pome
export class User {
    fun init(name, email) {
        this.name = name;
        this.email = email;
    }
    
    fun display() {
        print(this.name, "-", this.email);
    }
}
```

### Exporting Variables

```pome
// config.pome
export var DEFAULT_TIMEOUT = 5000;
export var MAX_RETRIES = 3;
```

## Importing Modules

Use the `import` keyword to load modules:

```pome
import math_utils;
```

This imports the module and makes its exported content available.

### Using Imported Content

Access exported items using dot notation:

```pome
import math_utils;

var result = math_utils.add(5, 3);
print(result);  // Output: 8
```

### Importing Classes

```pome
import models;

var user = models.User("Alice", "alice@example.com");
user.display();
```

## Module Paths

### Relative Paths

Specify the path to the module file:

```pome
import ../utils/math_utils;
import ./local_config;
import data/database;
```

Patterns:
- `../` - Parent directory
- `./` - Current directory
- `folder/file` - Subdirectory

### Module Names

The module name is the filename without `.pome`:

```
String "models.pome" → import models;
String "math_utils.pome" → import math_utils;
String "folder/handler.pome" → import folder/handler;
```

## Namespace Organization

Each module creates its own namespace:

```pome
// string_utils.pome
export fun trim(str) {
    // Remove whitespace
    return str;
}

export fun toUpper(str) {
    return str;
}
```

```pome
// main.pome
import string_utils;

var text = "  hello  ";
print(string_utils.trim(text));    // Uses trim from string_utils
print(string_utils.toUpper(text));  // Uses toUpper from string_utils
```

## Module Patterns

### Utility Modules

Group related functions:

```pome
// utils/array_utils.pome
export fun sum(arr) {
    var total = 0;
    for (var i = 0; i < len(arr); i = i + 1) {
        total = total + arr[i];
    }
    return total;
}

export fun average(arr) {
    if (len(arr) == 0) return 0;
    return sum(arr) / len(arr);
}

export fun max(arr) {
    if (len(arr) == 0) return nil;
    var maximum = arr[0];
    for (var i = 1; i < len(arr); i = i + 1) {
        if (arr[i] > maximum) {
            maximum = arr[i];
        }
    }
    return maximum;
}
```

```pome
// main.pome
import utils/array_utils;

var numbers = [1, 2, 3, 4, 5];
print(array_utils.sum(numbers));      // Output: 15
print(array_utils.average(numbers));  // Output: 3
print(array_utils.max(numbers));      // Output: 5
```

### Model/Class Modules

Define classes in separate modules:

```pome
// models/book.pome
export class Book {
    fun init(title, author, year) {
        this.title = title;
        this.author = author;
        this.year = year;
    }
    
    fun describe() {
        return this.title + " by " + this.author + " (" + this.year + ")";
    }
}
```

```pome
// main.pome
import models/book;

var book = models.Book("1984", "George Orwell", 1949);
print(book.describe());
```

### Configuration Modules

Store settings and constants:

```pome
// config.pome
export var DATABASE_URL = "localhost:5432";
export var API_KEY = "secret-key-123";
export var LOG_LEVEL = "INFO";

export var RETRY_CONFIG = {
    max_attempts: 3,
    timeout_ms: 5000,
    backoff_ms: 1000
};
```

```pome
// main.pome
import config;

print("Database:", config.DATABASE_URL);
print("Max attempts:", config.RETRY_CONFIG.max_attempts);
```

### Data Access Modules

Encapsulate data operations:

```pome
// data/user_store.pome
var users = [
    {id: 1, name: "Alice", email: "alice@example.com"},
    {id: 2, name: "Bob", email: "bob@example.com"}
];

export fun getUser(id) {
    for (var i = 0; i < len(users); i = i + 1) {
        if (users[i].id == id) {
            return users[i];
        }
    }
    return nil;
}

export fun addUser(user) {
    users = users + [user];
}

export fun getAllUsers() {
    return users;
}
```

```pome
// main.pome
import data/user_store;

var user = data/user_store.getUser(1);
print(user.name);

data/user_store.addUser({id: 3, name: "Charlie", email: "charlie@example.com"});
```

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
