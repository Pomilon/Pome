# Advanced Topics

Explore advanced Pome language concepts and patterns.

## Closures in Depth

### Understanding Lexical Scope

Closures capture variables from their defining scope:

```pome
var globalX = "global";

fun outer() {
    var outerX = "outer";
    
    return fun() {
        var innerX = "inner";
        print(globalX, outerX, innerX);
    };
}

var closure = outer();
closure();  // Output: global outer inner
```

### Practical Closure Examples

#### State Management

```pome
fun makeStateful() {
    var state = 0;
    
    return {
        increment: fun() {
            state = state + 1;
        },
        get: fun() {
            return state;
        },
        reset: fun() {
            state = 0;
        }
    };
}

var counter = makeStateful();
counter.increment();
counter.increment();
print(counter.get());  // Output: 2
counter.reset();
print(counter.get());  // Output: 0
```

#### Partial Application

```pome
fun multiply(a) {
    return fun(b) {
        return a * b;
    };
}

var double = multiply(2);
var triple = multiply(3);

print(double(5));  // Output: 10
print(triple(5));  // Output: 15
```

#### Decorators

```pome
fun withLogging(fn) {
    return fun(x) {
        print("Calling function with argument:", x);
        var result = fn(x);
        print("Function returned:", result);
        return result;
    };
}

var square = fun(x) { return x * x; };
var loggedSquare = withLogging(square);

loggedSquare(5);
```

Output:
```
Calling function with argument: 5
Function returned: 25
```

## Advanced Function Patterns

### Function Composition

```pome
fun compose(f, g) {
    return fun(x) {
        return f(g(x));
    };
}

fun add5(x) {
    return x + 5;
}

fun double(x) {
    return x * 2;
}

var addThenDouble = compose(double, add5);
print(addThenDouble(3));  // (3 + 5) * 2 = 16
```

### Pipeline Pattern

```pome
fun pipe(value, fns) {
    var result = value;
    for (var i = 0; i < len(fns); i = i + 1) {
        result = fns[i](result);
    }
    return result;
}

fun double(x) { return x * 2; }
fun addTen(x) { return x + 10; }
fun stringify(x) { return "Result: " + x; }

var result = pipe(5, [double, addTen, stringify]);
print(result);  // Output: Result: 20
```

### Memoization

```pome
fun memoize(fn) {
    var cache = {};
    
    return fun(x) {
        var key = x;
        if (cache[key] == nil) {
            cache[key] = fn(x);
        }
        return cache[key];
    };
}

fun expensiveComputation(n) {
    print("Computing...");
    var result = 0;
    for (var i = 0; i < n; i = i + 1) {
        result = result + i;
    }
    return result;
}

var memoized = memoize(expensiveComputation);

print(memoized(5));  // Computing... Output: 10
print(memoized(5));  // No computing, uses cache. Output: 10
```

## Object-Oriented Patterns

### Inheritance Simulation

```pome
class Animal {
    fun init(name) {
        this.name = name;
    }
    
    fun speak() {
        print(this.name, "makes a sound");
    }
}

class Dog {
    fun init(name, breed) {
        this.name = name;
        this.breed = breed;
    }
    
    fun speak() {
        print(this.name, "barks!");
    }
    
    fun getInfo() {
        return this.name + " is a " + this.breed;
    }
}

var dog = Dog("Buddy", "Golden Retriever");
dog.speak();
print(dog.getInfo());
```

### Factory Pattern

```pome
fun createDatabase(type) {
    if (type == "memory") {
        return {
            name: "In-Memory DB",
            connect: fun() { print("Connected to memory"); },
            query: fun(sql) { print("Executing:", sql); }
        };
    } else if (type == "file") {
        return {
            name: "File DB",
            connect: fun() { print("Connected to file"); },
            query: fun(sql) { print("Executing:", sql); }
        };
    }
}

var memDb = createDatabase("memory");
memDb.connect();
memDb.query("SELECT * FROM users");
```

### Observer Pattern

```pome
class EventEmitter {
    fun init() {
        this.listeners = {};
    }
    
    fun on(event, callback) {
        if (this.listeners[event] == nil) {
            this.listeners[event] = [];
        }
        this.listeners[event] = this.listeners[event] + [callback];
    }
    
    fun emit(event, data) {
        if (this.listeners[event] != nil) {
            for (var i = 0; i < len(this.listeners[event]); i = i + 1) {
                this.listeners[event][i](data);
            }
        }
    }
}

var emitter = EventEmitter();

emitter.on("message", fun(data) {
    print("Handler 1 received:", data);
});

emitter.on("message", fun(data) {
    print("Handler 2 received:", data);
});

emitter.emit("message", "Hello World");
```

Output:
```
Handler 1 received: Hello World
Handler 2 received: Hello World
```

## Advanced Data Structure Patterns

### Linked List

```pome
class Node {
    fun init(value) {
        this.value = value;
        this.next = nil;
    }
}

class LinkedList {
    fun init() {
        this.head = nil;
    }
    
    fun append(value) {
        var newNode = Node(value);
        
        if (this.head == nil) {
            this.head = newNode;
        } else {
            var current = this.head;
            while (current.next != nil) {
                current = current.next;
            }
            current.next = newNode;
        }
    }
    
    fun traverse() {
        var current = this.head;
        while (current != nil) {
            print(current.value);
            current = current.next;
        }
    }
}

var list = LinkedList();
list.append(1);
list.append(2);
list.append(3);
list.traverse();
```

### Tree Structure

```pome
class TreeNode {
    fun init(value) {
        this.value = value;
        this.left = nil;
        this.right = nil;
    }
}

class BinarySearchTree {
    fun init() {
        this.root = nil;
    }
    
    fun insert(value) {
        if (this.root == nil) {
            this.root = TreeNode(value);
        } else {
            this._insertNode(this.root, value);
        }
    }
    
    fun _insertNode(node, value) {
        if (value < node.value) {
            if (node.left == nil) {
                node.left = TreeNode(value);
            } else {
                this._insertNode(node.left, value);
            }
        } else {
            if (node.right == nil) {
                node.right = TreeNode(value);
            } else {
                this._insertNode(node.right, value);
            }
        }
    }
    
    fun inOrder(node) {
        if (node != nil) {
            this.inOrder(node.left);
            print(node.value);
            this.inOrder(node.right);
        }
    }
}

var tree = BinarySearchTree();
tree.insert(5);
tree.insert(3);
tree.insert(7);
tree.insert(1);
tree.insert(4);
tree.inOrder(tree.root);  // Output: 1 3 4 5 7
```

## Meta-Programming Patterns

### Reflection

```pome
fun describeObject(obj) {
    print("Object type:", type(obj));
    
    if (type(obj) == "table") {
        print("Keys: ");
        // Note: Pome doesn't have built-in key enumeration
        // This is a limitation to be aware of
    }
    
    if (type(obj) == "instance") {
        print("Instance of some class");
    }
}

var person = {name: "Alice", age: 30};
describeObject(person);
```

### Dynamic Method Calling

```pome
fun callMethod(obj, methodName, args) {
    var method = obj[methodName];
    if (method == nil) {
        print("Method not found:", methodName);
        return nil;
    }
    return method(args);
}

var calculator = {
    add: fun(nums) {
        var sum = 0;
        for (var i = 0; i < len(nums); i = i + 1) {
            sum = sum + nums[i];
        }
        return sum;
    }
};

var result = callMethod(calculator, "add", [1, 2, 3]);
print(result);  // Output: 6
```

## Performance Optimization Patterns

### Lazy Evaluation

```pome
fun lazyRange(n) {
    return {
        index: 0,
        next: fun() {
            if (this.index < n) {
                var val = this.index;
                this.index = this.index + 1;
                return val;
            }
            return nil;
        }
    };
}

var range = lazyRange(3);
print(range.next());  // 0
print(range.next());  // 1
print(range.next());  // 2
print(range.next());  // nil
```

### Tail Call Optimization Pattern

Since Pome doesn't optimize tail calls, use an iterative approach:

```pome
// Instead of recursive
fun factorialRecursive(n) {
    if (n <= 1) return 1;
    return n * factorialRecursive(n - 1);
}

// Use iterative
fun factorial(n) {
    var result = 1;
    for (var i = 2; i <= n; i = i + 1) {
        result = result * i;
    }
    return result;
}

print(factorial(5));  // Output: 120
```

## Best Practices for Advanced Code

1. **Keep it readable**: Clever code is hard to maintain
   ```pome
   // Clever but confusing
   var x = pipe(5, [double, addTen, stringify]);
   
   // Clear
   var doubled = double(5);
   var added = addTen(doubled);
   var result = stringify(added);
   ```

2. **Document complex patterns**: Explain non-obvious code
   ```pome
   // Memoization: caches results to avoid recomputation
   fun memoize(fn) {
       var cache = {};
       // ...
   }
   ```

3. **Test edge cases**: Advanced patterns need thorough testing
   ```pome
   // Test memoization with edge cases
   var memoized = memoize(expensiveFunc);
   print(memoized(0));      // First call
   print(memoized(0));      // From cache
   print(memoized(1));      // Different input
   ```

4. **Avoid over-engineering**: Don't use advanced patterns unless needed
   ```pome
   // Probably overkill
   var sumFunc = memoize(compose(reduce, map));
   
   // Simple and clear
   fun sum(nums) {
       var total = 0;
       for (var i = 0; i < len(nums); i = i + 1) {
           total = total + nums[i];
       }
       return total;
   }
   ```

---

**Next**: [Architecture](12-architecture.md)  
**Back to**: [Documentation Home](README.md)
