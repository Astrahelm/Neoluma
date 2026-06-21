# Neoluma First Alpha Specification

This specification is a description of details that will be available in Neoluma Alpha Release v0.1 since the package publication. <br>
Package publication will not contain **ALL** the promised features on day 1, and they will be added in reoccuring updates. <br>
During Alpha, **the syntax is a subject to change**. Every update **can break** your program. Hence it's currently not recommended for production use, rather experiments and simple programs.

## 🔹 Data Types

Neoluma supports a variety of data types to handle different scenarios.

| Type      | Description                                                                 |
|-----------|-----------------------------------------------------------------------------|
| `int (8/16/32/64/128)`     | Represents integers (whole numbers), including negatives.                  |
| `uint (8/16/32/64/128)`    | Represents unsigned integers (whole numbers ≥ 0).                          |
| `float (32/64)`   | A floating-point number based on the IEEE 754 standard.                    |
| `number`  | A high-precision number type using base and exponent (10^x). Ideal for accurate math. |
| `str`  | A text string. Supports inline interpolation with `${expression}`.         |
| `bool`    | Boolean value (`true` or `false`). Implicit conversion: `0` is `false`, non-zero is `true`. |
| `array`   | Ordered collection of elements. Can mix types.                             |
| `set`     | Unordered collection of unique values.                                     |
| `dict`    | Key-value map (dictionary). Keys are unique; values can be any type.       |
| `void`    | Represents the absence of a value. Typically used as a return type.        |
| `result`  | A wrapper for success or error values, like `try-catch`.                   |

### Nullable Types
Values can be written with `?` to the right of them, like `int?` or `string?`, to mark that they are nullable. Nullable types can hold either a value or `null`.
Values without it **cannot** be named null, and throw a `NullAssignmentToNonNullable` error.

### Variable Sizes
Sizes of variables can be defined with `[<number>]` or premade types in the table, like `x: int8 = 255;` or `x: int[3] = 5;`, if you need a more custom amount of bytes.
<br>**Note:** It's much more preferred to use default types like `int`, `float` and etc. 
+-unless you know what you're doing. 

### Examples
```neoluma
// Neoluma does NOT have let/var. Ever. It's declared without it. More on that later.

// Numeric types
a: int = 10
b: float = 3.14
c: number = 1.23e10

// Strings
name: str = "Tsuki"
greeting: str = "Hello, ${name}!"

// Booleans
flag: bool = true
isReady: bool = 0  // Implicitly false

// Collections
nums: array = [1, 2, 3, "four"]
unique: set = {1, 2, 2, 3}  // Contains 1, 2, 3
constant_list: tuple = (1, 5, 6, 6, 12) // This tuple is not modifiable
info: dict = {"name": "Tsuki", "age": 18}

// Nullable types
age: int? = null

// Functions
fn greet(person: dict) -> void {
    print("Hello, ${person['name']}!")
}

res: result = greet(info)
```
## 🔹 Functions

Functions in Neoluma are declared using either `fn` or `function` (if `verbose=true` in the compiler settings). They encapsulate logic and make your code reusable.

### Basic Syntax
```neoluma
fn add(a: int, b: int) -> int {
    return a + b
}
```
- Use `-> <type>` to specify the return type.
- Use `: <type>` for parameters to specify the input type.
- Semicolons are optional but recommended (Neoluma accepts either semicolons or newlines, this actually works for all statements in code).
- Functions without a return type default to `void` or whatever is returned.

### One-Liners
For simple single-line expressions, you can use `=>`:
```neoluma
fn subtract(a, b) -> int => a - b
```
But if you need an anonymous quick functions, it would be better to use lambdas for that. This is just a syntactic sugar for functions that only contain 1 statement.

### Modifiers
Functions can contain modifiers that modify the function scope.

***Visibility scope***
<br>Modifies the function's access from other files or call expressions.
```neoluma
public fn greet(name: str) {
    print("Hello, ${name}!")
}

protected fn calculate() -> int {
    return 42
}

private fn secret() {
    print("This is private")
}
```
- `public`: Accessible from anywhere.
- `protected`: Accessible within the class and its subclasses.
- `private`: Accessible only within the class/files.

***Asynchronous functions***
<br> Allow functions it to finish later instead of immediately. Basically schedules the operation until it's called with `await` keyword.
```neoluma
#import "net"

async fn fetchText(url: str) -> str {
    response := await net.requests.get(url);
    return await response;
}

// Variant 1: assigning the task to a variable and then call with it 
text := fetchText("https://raw.githubusercontent.com/Rick-Lang/rickroll-lang/main/README.md");
res := await text; // Received result

// Variant 2: calling directly
res := await fetchText("https://raw.githubusercontent.com/Rick-Lang/rickroll-lang/main/README.md");
```
- `async`: Modifier to make function asynchronous
- `await`: Keyword that allows us to receives output from scheduled operation

***Other modifiers***
<br> Let's address other modifiers that remain to get implemented:
- **Static** <br>
  A function that can be called without creating the object
  ``` neoluma
  class Test {
      static fn hello() {
          print("Hi");
      }
  }

  Test.hello(); // "Hi"
  ```
- **Override** <br>
  Function that overrides the behavior of other function from inherited class
  ```neoluma
  class Animal {
    fn speak() -> str {
        return "..."
    }
  }

  class Cat <- Animal {
    // overridden function
    override fn speak() -> str {
        return "Meow!"
    }
  }

  cat := Cat();
  cat.speak(); // Meow!
  ```
- **Debug** <br>
  Function that only exists if current build mode is Debug. If it's Release, they are removed.
  ```neoluma
  #import "fs"

  debug fn log(message: str) {
    fs.write("log.txt", message)
  }

  fn register() -> result {
    //...
    // gone if compiled as Release
    log("Registration successful!") 
    return Ok;
  }
  ```
- **Intrinsic** <br>
  Modifier that works in standard libraries for LLVM libc bindings. Function blocks are ignored. Not planned to be used in a user space, will throw an error if you do that.

### Lambdas
Inline anonymous functions can be written as:
```neoluma
(x, y) => x + y
```
which takes `x, y` as arguments for operation to be operated in a block or one-liner.

### Docstrings
Functions can have documentation above them using `///`:
```neoluma
/// Adds two numbers
fn add(a: int, b: int) -> int {
    return a + b
}
```
- Use `///` for single-line comments.
- A multiline docstring is formed by stacking multiple `///` lines above the function.
- LSP must read these properly.

### Decorators
Decorators modify function behavior or add metadata. Use `@` above them:
```neoluma
@entry
fn main() {
    print("Program started")
}

@unsafe
fn riskyOperation() {
    // Low-level code
}

@comptime
fn compileTimeFunction() {
    print("This runs at compile time!")
}
```
- `@entry`: Marks the entry point of the program.
- `@unsafe`: Allows unsafe or raw memory operations.
- `@comptime`: Executes the function at compile time.

You can create your own with `decorator` keyword instead of `fn`, while calling `base()` as a function being modified.
```neoluma
#import "std.time"

decorator measure_time() {
    timer.start();
    value := base(...);
    result := timer.end();
    print("Function executed in ${result} ms");
    return value
}

@measure_time
fn add(x, y) => x + y

print(add(2, 3)) // Function executed in 0.0000004 ms
                 // 5
```

## 🔹 Classes

Neoluma uses a class-based object-oriented model inspired by C# and JavaScript.

### Declaring Classes
```neoluma
class Animal {
    fn speak() {
        print("Sound")
    }
}
```
- Classes can have methods, fields, and constructors.

### Constructors
```neoluma
class Dog {
    Dog(name: string) {
        self.name = name
    }
}
```
- `self` refers to the current instance.

### Inheritance
Use `<-` to inherit:
```neoluma
class Dog <- Animal {
    override fn speak() {
        print("Bark")
    }
}
```
- The `override` keyword, for reminder, is used to redefine methods from the parent class.

### Static Methods
```neoluma
class Utils {
    static fn add(a, b) -> int {
        return a + b
    }
}

Utils.add(1, 2)
```
- Again, static methods belong to the class, not instances.

## 🔹 Namespaces
Namespaces are named scopes that group related code into a single logical unit. A namespace may span multiple files and can be imported through a single path, allowing the compiler to resolve its contents without requiring direct file-by-file imports.

A namespace is declared with the `namespace` keyword followed by its name. All declarations inside the block belong to that namespace.
```neoluma
namespace std {
    func print(text: String) { ... }
}

std.print("Hello")
```
Namespaces can also be imported the same way as native modules, since native modules are placed inside a namespace by default.
```neoluma
#import std.net as net

req := net.requests.get("http://localhost:8080/api/v1/carList");
```

## 🔹 Control Flow
Order in which program decides on what to do next

### Conditionals
```neoluma
if (x > 10) {
    print("Greater than 10")
} else {
    print("10 or less")
}

if (fruit != "apple") print("Why am i not an apple :c")
else print("Yay, i am an apple!")
```
- Conditions are written in parentheses `(...)`.
- Blocks are enclosed in `{}`.
- You don't need curly braces, if there's only one statement

### Switch
```neoluma
switch (value) {
    case 1:
        print("One")
    case 2:
        print("Two")
    default:
        print("Other")
}
```
- No `break` needed; no fallthrough.

**Why No `match`?**
Neoluma does not include a separate `match` expression. Instead, its functionality is fully covered by the `switch` statement. Here’s why:
- `switch` supports constants and strings.
- It avoids ambiguous behaviors like fallthrough by default.
- It’s cleaner and easier to parse than pattern-matching syntax.

This keeps the language simpler and easier to learn. (It’s already painful to shape the language together when you try to please everyone. This here is a compromise.)

### Loops
```neoluma
// : -> in (verbose=true in compiler settings)
for (i in range(10)) {
    print(i)
}

for (item : items) {
    print(item)
}

while (condition) {
    // code
}

loop {
    // infinite loops, recommended instead of while(true)
}
```
- `for` loops support both range-based and collection-based iteration.
- Old `int i = 0; i < 10; i++` stuff have been dropped.

### Try-Catch
```neoluma
try {
    riskyFunction()
} catch (e) {
    print("Error: ${e}")
}

fn riskyFunction() {
    throw "Something went wrong"
}
```
- `try`: Defines a block of code where exceptions may occur.
- `catch`: Handles exceptions thrown in the `try` block.
- `throw`: Used to raise an exception.

## 🔹 Preprocessors
Preprocessors are compile-time statements that transform source code before the main compilation process.
<br>In current stage, Neoluma has 3 preprocessors to choose from:
- `#import`: Allows to import native packages, namespaces or files from relative paths to receive internal functions, classes, variables, etc.
- `#unsafe`: Marks the file as unsafe (that it could handle memory management logic unusual to the chosen memory management node). Allows using pointers.
- `#macro`: Defines a variable to a value, and swaps the variable to the value everywhere where it was imported in the code mentioned. For example, `#macro X 7`.

### Imports
Imports are compile-time directives that make declarations from another native package, namespace, or file available in the current source file.

In Neoluma, imports are declared with `#import` and are resolved before the main compilation stage. They are used to access external functions, classes, variables, and other declarations without duplicating code.

Imports may target:
```neoluma
#import "std" // native packages,
#import "std.net" // namespaces
#import "scripts/math" // or files from relative paths (no .nm)
```

## 🔹 Package system 
Neoluma uses a package system to install and manage external libraries, tools, and language packs.

Packages are declared in the project file and resolved before compilation.
Source code only imports modules, while the project configuration defines where those modules come from.

### Dependencies
Dependencies are declared in the `.nlp` file:

```toml
[dependencies]
opengl_nm = "0.1.0"
rest = "1.2.0"
```

You personally, as a user, can add these via `neoluma pm add <author:package-id>` in CLI

### Purpose
The package system is used for:
- installing external libraries
- resolving project dependencies
- making package modules available to imports
- distributing standard libraries and language packs
### Notes
- Relative imports are part of the import system, not the package system.
- Package resolution is project-wide.
- Missing packages must produce a compiler error.

## 🔹 Project structure 

A Neoluma project is described by a .nlp file.
This file defines the project metadata, source folder, dependencies, tasks, tests, and language packs.

### Basic Example
```
[project]
name = "Example"
version = "0.1.0"
authors = ["Tsuki"]
license = "custom"
sourceFolder = "src/"
buildFolder = ".build/"
output = "exe"

[dependencies]
std = "0.1.0"

[tasks]
dev = "neoluma run --debug"

[tests]
frontend = "tests/"
```

### Purpose
The project file is used to define:

- project metadata
- source file location
- output settings
- dependencies
- tasks
- tests

### Source Layout
A typical project looks like this:
```
MyProject/
├─ myproject.nlp
├─ src/
│  └─ main.nm
├─ tests/
└─ .build/
```
### Notes
- The compiler reads the .nlp file before collecting project files.
- Source files are collected from sourceFolder.
- Project-wide settings belong in .nlp, not in source code.
- Build mode, package dependencies, and language packs are project concerns.
