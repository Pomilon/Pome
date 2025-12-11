# Collections

Collections allow you to store and organize multiple values together.

## Lists

Lists are ordered collections of values:

```pome
var numbers = [1, 2, 3, 4, 5];
var mixed = [1, "two", 3.0, true, nil];
```

### Accessing Elements

Use square brackets with an index (0-based):

```pome
var fruits = ["apple", "banana", "orange"];
print(fruits[0]);  // Output: apple
print(fruits[1]);  // Output: banana
print(fruits[2]);  // Output: orange
```

### Negative Indexing

Negative indices count from the end:

```pome
var fruits = ["apple", "banana", "orange"];
print(fruits[-1]);  // Output: orange (last element)
print(fruits[-2]);  // Output: banana (second to last)
```

### Getting List Length

Use the `len()` function:

```pome
var items = [1, 2, 3, 4, 5];
print(len(items));  // Output: 5
```

### Modifying Lists

Assign new values to elements:

```pome
var numbers = [1, 2, 3];
numbers[1] = 20;
print(numbers);  // Output: [1, 20, 3]
```

### Creating Empty Lists

```pome
var empty = [];
print(len(empty));  // Output: 0
```

### List Concatenation

Combine lists with the `+` operator:

```pome
var list1 = [1, 2];
var list2 = [3, 4];
var combined = list1 + list2;
print(combined);  // Output: [1, 2, 3, 4]
```

### Iterating Over Lists

```pome
var items = ["a", "b", "c"];
for (var i = 0; i < len(items); i = i + 1) {
    print(items[i]);
}
```

Output:
```
a
b
c
```

### List Slicing

Extract a portion of a list:

```pome
var numbers = [1, 2, 3, 4, 5];
var slice = numbers[1:4];  // Elements 1, 2, 3
print(slice);  // Output: [2, 3, 4]
```

Slicing syntax: `list[start:end]`
- `start`: First index to include (default 0)
- `end`: First index to exclude

```pome
var numbers = [1, 2, 3, 4, 5];
print(numbers[0:2]);    // Output: [1, 2]
print(numbers[2:]);     // Output: [3, 4, 5]
print(numbers[:3]);     // Output: [1, 2, 3]
```

## Tables

Tables are collections of key-value pairs (dictionaries):

```pome
var person = {
    name: "Alice",
    age: 30,
    city: "New York"
};
```

### Accessing Values

Use bracket notation or dot notation:

```pome
var person = {name: "Bob", age: 25};
print(person["name"]);  // Output: Bob
print(person.name);     // Output: Bob
```

### Modifying Tables

```pome
var person = {name: "Charlie", age: 35};
person.age = 36;
person["city"] = "Boston";
print(person);
```

### Adding New Keys

Simply assign to a new key:

```pome
var settings = {};
settings.theme = "dark";
settings["language"] = "English";
print(settings);
```

### Checking for Keys

```pome
var person = {name: "Diana", age: 28};

// Pattern: try accessing and check for nil
if (person.email != nil) {
    print("Email:", person.email);
}
```

### Iterating Over Tables

You'll need to use specific patterns since Pome doesn't have built-in iteration:

```pome
var person = {name: "Eve", age: 40};

// Tables don't have built-in iteration in the current version
// You can access known keys
print(person.name);
print(person.age);
```

### Table Size

```pome
var data = {a: 1, b: 2, c: 3};
print(len(data));  // Output: 3
```

### Nested Tables

Tables can contain other tables:

```pome
var user = {
    name: "Frank",
    address: {
        street: "123 Main St",
        city: "Portland",
        zip: "97201"
    }
};

print(user.address.city);  // Output: Portland
user.address.zip = "97202";
```

### Table Patterns

Working with collections of objects:

```pome
var users = [
    {name: "Alice", age: 30},
    {name: "Bob", age: 25},
    {name: "Charlie", age: 35}
];

print(users[0].name);  // Output: Alice
print(users[1].age);   // Output: 25
```

## Common Collection Operations

### Searching for Values

```pome
var numbers = [1, 2, 3, 4, 5];
var target = 3;
var found = false;

for (var i = 0; i < len(numbers); i = i + 1) {
    if (numbers[i] == target) {
        found = true;
    }
}

print(found);  // Output: true
```

### Filtering

```pome
var numbers = [1, 2, 3, 4, 5, 6];
var evens = [];

for (var i = 0; i < len(numbers); i = i + 1) {
    if (numbers[i] % 2 == 0) {
        evens = evens + [numbers[i]];
    }
}

print(evens);  // Output: [2, 4, 6]
```

### Mapping

```pome
var numbers = [1, 2, 3, 4];
var doubled = [];

for (var i = 0; i < len(numbers); i = i + 1) {
    doubled = doubled + [numbers[i] * 2];
}

print(doubled);  // Output: [2, 4, 6, 8]
```

### Finding Maximum

```pome
var numbers = [5, 2, 8, 1, 9, 3];
var max = numbers[0];

for (var i = 1; i < len(numbers); i = i + 1) {
    if (numbers[i] > max) {
        max = numbers[i];
    }
}

print(max);  // Output: 9
```

### Summing Values

```pome
var numbers = [1, 2, 3, 4, 5];
var total = 0;

for (var i = 0; i < len(numbers); i = i + 1) {
    total = total + numbers[i];
}

print(total);  // Output: 15
```

## Helper Functions with Collections

### Count Occurrences

```pome
fun countOccurrences(list, value) {
    var count = 0;
    for (var i = 0; i < len(list); i = i + 1) {
        if (list[i] == value) {
            count = count + 1;
        }
    }
    return count;
}

var items = [1, 2, 3, 2, 4, 2];
print(countOccurrences(items, 2));  // Output: 3
```

### Reverse List

```pome
fun reverse(list) {
    var result = [];
    for (var i = len(list) - 1; i >= 0; i = i - 1) {
        result = result + [list[i]];
    }
    return result;
}

print(reverse([1, 2, 3]));  // Output: [3, 2, 1]
```

### Flatten Nested Lists

```pome
fun flatten(list) {
    var result = [];
    for (var i = 0; i < len(list); i = i + 1) {
        var item = list[i];
        if (type(item) == "list") {
            result = result + flatten(item);
        } else {
            result = result + [item];
        }
    }
    return result;
}

var nested = [1, [2, 3], [4, [5, 6]]];
print(flatten(nested));  // Output: [1, 2, 3, 4, 5, 6]
```

## Best Practices

1. **Use meaningful variable names**: `users` instead of `u`

2. **Check bounds before accessing**:
   ```pome
   var items = [1, 2, 3];
   if (index >= 0 and index < len(items)) {
       print(items[index]);
   }
   ```

3. **Use consistent key names in tables**:
   ```pome
   var user1 = {name: "Alice", age: 30};
   var user2 = {name: "Bob", age: 25};
   ```

4. **Keep table keys simple**: Use lowercase with underscores
   ```pome
   var config = {
       max_retries: 3,
       timeout_ms: 5000
   };
   ```

5. **Use lists for ordered data, tables for structured data**:
   ```pome
   var scores = [100, 95, 88];      // List - ordered values
   var person = {name: "Alice", age: 30};  // Table - structured data
   ```

6. **Document complex data structures**:
   ```pome
   // users is a list of tables with name, email, age
   var users = [
       {name: "Alice", email: "alice@example.com", age: 30}
   ];
   ```

---

**Next**: [Operators](07-operators.md)  
**Back to**: [Documentation Home](README.md)
