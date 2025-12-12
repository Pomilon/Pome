# Object-Oriented Programming

Pome supports object-oriented programming through classes and objects.

## Classes and Objects

### Defining a Class

Use the `class` keyword to define a class:

```pome
class Dog {
    fun init(name) {
        this.name = name;
        this.sound = "Woof";
    }
    
    fun speak() {
        print(this.name, "says", this.sound);
    }
}
```

### Creating Objects

Create an object by calling the class like a function:

```pome
var dog = Dog("Buddy");
dog.speak();  // Output: Buddy says Woof
```

## The init Method

The `init` method is a constructor - it runs when an object is created:

```pome
class Person {
    fun init(name, age) {
        this.name = name;
        this.age = age;
    }
}

var person = Person("Alice", 30);
print(person.name);  // Output: Alice
print(person.age);   // Output: 30
```

## Instance Variables

Instance variables store data specific to each object. Access them with `this`:

```pome
class Counter {
    fun init() {
        this.count = 0;
    }
    
    fun increment() {
        this.count = this.count + 1;
    }
    
    fun getCount() {
        return this.count;
    }
}

var c1 = Counter();
var c2 = Counter();

c1.increment();
c1.increment();
c2.increment();

print(c1.getCount());  // Output: 2
print(c2.getCount());  // Output: 1
```

Each object has its own set of instance variables.

## Methods

Methods are functions defined inside a class:

```pome
class Rectangle {
    fun init(width, height) {
        this.width = width;
        this.height = height;
    }
    
    fun area() {
        return this.width * this.height;
    }
    
    fun perimeter() {
        return 2 * (this.width + this.height);
    }
}

var rect = Rectangle(5, 10);
print(rect.area());       // Output: 50
print(rect.perimeter());  // Output: 30
```

## The `this` Keyword

`this` refers to the current object:

```pome
class Dog {
    fun init(name) {
        this.name = name;
    }
    
    fun introduce() {
        print("I am " + this.name);
    }
}

var dog = Dog("Max");
dog.introduce();  // Output: I am Max
```

## Accessing Object Properties

Access object properties using dot notation:

```pome
class Person {
    fun init(name) {
        this.name = name;
    }
}

var person = Person("Bob");
print(person.name);      // Output: Bob
person.name = "Robert";  // Modify property
print(person.name);      // Output: Robert
```

## Method Chaining

Methods can return `this` to enable chaining:

```pome
class Calculator {
    fun init() {
        this.result = 0;
    }
    
    fun add(x) {
        this.result = this.result + x;
        return this;
    }
    
    fun multiply(x) {
        this.result = this.result * x;
        return this;
    }
    
    fun getResult() {
        return this.result;
    }
}

var calc = Calculator();
var answer = calc.add(5).multiply(3).add(2).getResult();
print(answer);  // Output: 17 ((5 * 3) + 2)
```

## Stateful Objects

Classes are useful for maintaining state:

```pome
class BankAccount {
    fun init(owner, balance) {
        this.owner = owner;
        this.balance = balance;
    }
    
    fun deposit(amount) {
        this.balance = this.balance + amount;
        print(this.owner, "deposited", amount);
    }
    
    fun withdraw(amount) {
        if (amount > this.balance) {
            print("Insufficient funds");
            return false;
        }
        this.balance = this.balance - amount;
        print(this.owner, "withdrew", amount);
        return true;
    }
    
    fun getBalance() {
        return this.balance;
    }
}

var account = BankAccount("Alice", 1000);
account.deposit(500);     // Alice deposited 500
account.withdraw(200);    // Alice withdrew 200
print(account.getBalance());  // Output: 1300
```

## Encapsulation Pattern

While Pome doesn't enforce access modifiers (private/public), you can follow conventions.

```pome
class Stack {
    fun init() {
        this._items = [];  // Convention: _items is private
    }
    
    fun push(value) {
        this._items = this._items + [value];
    }
    
    fun pop() {
        if (len(this._items) == 0) {
            return nil;
        }
        var item = this._items[-1];
        this._items = this._items[:len(this._items)-1];
        return item;
    }
    
    fun isEmpty() {
        return len(this._items) == 0;
    }
}

var stack = Stack();
stack.push(1);
stack.push(2);
print(stack.pop());  // Output: 2
print(stack.pop());  // Output: 1
print(stack.isEmpty());     // Output: true
```

## Composition

Objects can contain other objects:

```pome
class Engine {
    fun init(horsepower) {
        this.horsepower = horsepower;
    }
}

class Car {
    fun init(brand, engine) {
        this.brand = brand;
        this.engine = engine;
    }
    
    fun getHorsepower() {
        return this.engine.horsepower;
    }
}

var engine = Engine(300);
var car = Car("Ferrari", engine);
print(car.getHorsepower());  // Output: 300
```

## Checking Object Type

Use `type()` to check an object's type:

```pome
class MyClass {
    fun init() {
    }
}

var obj = MyClass();
print(type(obj));  // Output: instance
```

To check the class name, access the class through the instance:

```pome
// Note: Pome doesn't have direct class name introspection
// You can store the class name as a property:

class Dog {
    fun init(name) {
        this.name = name;
        this._className = "Dog";
    }
}

var dog = Dog("Rex");
print(dog._className);  // Output: Dog
```

## Common Patterns

### Getters and Setters

```pome
class Temperature {
    fun init(celsius) {
        this._celsius = celsius;
    }
    
    fun getCelsius() {
        return this._celsius;
    }
    
    fun getFahrenheit() {
        return this._celsius * 9 / 5 + 32;
    }
    
    fun setCelsius(value) {
        this._celsius = value;
    }
}

var temp = Temperature(0);
print(temp.getCelsius());    // Output: 0
print(temp.getFahrenheit()); // Output: 32
temp.setCelsius(100);
print(temp.getFahrenheit()); // Output: 212
```

### Factory Methods

```pome
class Point {
    fun init(x, y) {
        this.x = x;
        this.y = y;
    }
    
    fun distanceFromOrigin() {
        var dx = this.x;
        var dy = this.y;
        return (dx * dx + dy * dy) ^ 0.5;
    }
}

// Create points
var p1 = Point(3, 4);
print("Distance from origin:", p1.distanceFromOrigin()); // Output: Distance from origin: 5
```

### Object Comparison

Pome doesn't have built-in object comparison, but you can implement it:

```pome
class Color {
    fun init(r, g, b) {
        this.r = r;
        this.g = g;
        this.b = b;
    }
    
    fun equals(other) {
        return this.r == other.r and 
               this.g == other.g and 
               this.b == other.b;
    }
}

var c1 = Color(255, 0, 0);
var c2 = Color(255, 0, 0);
var c3 = Color(0, 255, 0);

print(c1.equals(c2));  // Output: true
print(c1.equals(c3));  // Output: false
```

## Best Practices

1. **Use meaningful class names**: `BankAccount` is clearer than `BA`

2. **Initialize all properties in init**: Prevents undefined property errors

   ```pome
   class Dog {
       fun init(name) {
           this.name = name;
           this.age = 0;  // Initialize here
       }
   }
   ```

3. **Keep classes focused**: A class should represent one concept

4. **Use method names that describe actions**: `withdraw()` not `w()`

5. **Consider composition over complex inheritance patterns**

6. **Document the expected behavior**: Use comments for non-obvious methods

   ```pome
   // Removes and returns the top item
   fun pop() {
       // ...
   }
   ```

---

**Next**: [Collections](06-collections.md)  
**Back to**: [Documentation Home](README.md)
