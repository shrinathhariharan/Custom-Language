# Custom Language

A custom interpreted language implemented in C++. It parses source code using a recursive descent parser into an Abstract Syntax Tree composed of C++ AST node objects (`Expr` and `Stmt`), which are then evaluated in a scoped environment.

---

## Features & Capabilities

### 1. Data Types & Values
The language supports four core data types:
* **`int`**: Integers (e.g., `42`, `-10`).
* **`dec`**: Floating point numbers / decimals (e.g., `3.14`, `0.001`).
* **`str`**: String literals enclosed in double quotes (e.g., `"Hello, World!"`). Supports escape sequences: `\n`, `\t`, `\"`, `\\`.
* **`bool`**: Boolean values (`true` and `false`).

---

### 2. Variable Declarations & Default Initialization
Variables are declared using their type keyword. Initialization is optional; if omitted, variables are initialized to their default value.

```custom
int a = 10
dec b = 3.14
str greeting = "Hello"
bool active = true

// Default initializations if initializer is omitted:
int uninitInt    // Defaults to 0
dec uninitDec    // Defaults to 0.0
str uninitStr    // Defaults to ""
bool uninitBool  // Defaults to false
```

---

### 3. Dynamic Arrays
Arrays are collections of uniform types. Declared using standard array syntax `type name[] = { ... }`.

```custom
// Array Declarations
int numbers[] = {1, 2, 3, 4, 5}
str colors[] = {"red", "green", "blue"}
bool flags[] = {true, false, true}

// Array Element Access (0-indexed)
print(numbers[0])

// Array Element Assignment
numbers[1] = 42

// Array member functions
print(numbers.size())       // 5: returns the current item count
numbers.push(6)             // appends 6; returns the new size
numbers.insert(1, 99)       // inserts 99 at index 1; returns the new size
int last = numbers.pop()    // removes and returns the last item (6)
print(numbers)              // [1, 99, 42, 3, 4, 5]
```

`push` and `insert` accept an item matching the array's declared type. `pop` reports an error when used on an empty array, and `insert` accepts indexes from `0` through `arr.size()` (inclusive).

### String member functions

Strings provide non-mutating member functions. `lower()` and `upper()` return a transformed string; assign their result to retain it.

```custom
str message = "Hello World"
int characterCount = message.length()  // 11; size() is an alias
int position = message.find("World")  // 6, or -1 when not found
bool hasHello = message.contains("Hello")
str lowercase = message.lower()
str uppercase = message.upper()
```

---

### 4. Operators & Expressions
Supported arithmetic, comparison, logical, and string operations with standard operator precedence:

* **Arithmetic Operators**: `+`, `-`, `*`, `/`, `%`, unary `-`
  * String concatenation is automatically supported with `+` if either operand is a string (e.g., `"Value: " + 10`).
  * Division by zero returns `0.0` safely; modulo by zero returns `0` (or `0.0`).
* **Comparison Operators**: `>`, `<`, `>=`, `<=`, `==`, `!=`
* **Logical Operators**: `&&`, `||`, `!` (short-circuiting)
* **Compound Assignment Operators**: `+=`, `-=`, `*=`, `/=`, `%=` (also supported for array elements)
* **Parentheses**: `(` and `)` for overriding operator precedence.

```custom
int score = 10
score += 5       // 15
score %= 4       // 3
score *= 2       // 6

int values[] = {4, 8}
values[1] %= 5   // [4, 3]

bool allowed = score >= 30 && !false
bool fallback = false || allowed
```

---

### 5. Control Flow Statements

#### `if` / `else if` / `else` Statements
Conditional execution based on condition evaluation.
```custom
if (x > 10) {
    print("Greater")
} else if (x == 10) {
    print("Equal")
} else {
    print("Smaller")
}
```

#### `while` Loops
Loops as long as the condition evaluates to `true`.
```custom
int count = 0
while (count < 5) {
    print(count)
    count += 1
}
```

#### `for` Loops
Standard 3-part `for` loop syntax with comma separators: `for (initializer, condition, increment) { ... }`
```custom
for (int i = 0, i < 5, i += 1) {
    print(i)
}
```

---

### 6. Functions & Scope
User-defined functions support optional return type and parameter type annotations, local variable scoping, and recursion.

```custom
// Function Definition
func int add(int a, int b) {
    return a + b
}

// Function Call
int sum = add(5, 10)
print(sum)
```
* Functions create a new nested variable scope upon execution.
* The `return` statement unwinds execution and returns a value to the caller.
* `void` functions do not return values

### 7. Classes and Objects

Classes contain typed fields, an optional constructor named after the class, and methods. Create an object with `ClassName variable = ClassName(...)`. Fields beginning with `_` are private and can only be accessed through `self` inside a class method.

```custom
class Point {
    int _x = 0
    int _y = 0
    int example = 0

    Point(self, x, y) {
        self._x = x
        self._y = y
    }

    func void hello() {
        print("Hello\n")
    }
}

Point point = Point(3, 4)
point.hello()
point.example += 1
```

---

### 8. Multi-File Imports

Include other `.txt` source files using the `import` keyword. Functions, classes, and top-level variables defined in imported files are namespaced using the module name (the filename without extension).

```custom
// helloworld.txt
func str helloWorld()
{
    return "Hello World!"
}

// main.txt
import "helloworld.txt"

print(helloworld.helloWorld()) // Outputs "Hello World!"
```

---

### 9. Built-in Functions

#### `print(...)`
Prints the evaluated string representation of an expression to standard output.
```custom
print("Hello World\n")
print(42 + 8)
```

#### `input(...)`
Reads a line of text from standard input. Accepts an optional prompt string to display before reading input.
```custom
str name = input("Enter your name: ")
print("Hello, " + name)
```

#### `toInt(value)`
Converts a value to an `int`. Strings are parsed numerically; booleans become `1` or `0`; decimals are truncated.
```custom
int a = toInt("42")     // 42
int b = toInt(3.99)     // 3  (truncated)
int c = toInt(true)     // 1
```

#### `toDec(value)`
Converts a value to a `dec` (double). Strings are parsed numerically; booleans become `1.0` or `0.0`.
```custom
dec a = toDec("3.14")   // 3.14
dec b = toDec(7)        // 7.0
dec c = toDec(false)    // 0.0
```

#### `toStr(value)`
Converts any value to its string representation (same as the string printed by `print`).
```custom
str a = toStr(42)       // "42"
str b = toStr(true)     // "true"
str c = toStr(3.14)     // "3.14"
```

#### `toBool(value)`
Converts a value to a `bool`.
* **`int` / `dec`**: `true` if the value is non-zero, `false` if zero.
* **`str`**: `true` if the string is non-empty, `false` if `""`.
```custom
bool a = toBool(1.0)    // true
bool b = toBool(0)      // false
bool c = toBool("hi")   // true
bool d = toBool("")     // false
```

#### `type(value)`
Returns the runtime type of a value as a `str`: `"int"`, `"dec"`, `"str"`, `"bool"`, `"array"`, or the class name for objects.
```custom
print(type(42))         // int
print(type(3.14))       // dec
print(type("hello"))    // str
print(type(true))       // bool
```

---

### 10. Comments & Syntax Rules
* **No Semicolons**: Semicolons (`;`) will throw a compiler error if present.
* **Single-line Comments**: Start with `//` and ignore all subsequent characters on that line.

```custom
// This is a single-line comment
int x = 5 // Inline comment
```

---

### 11. Standard Library

Standard library modules can be imported using `import module` or `import "module"`, and their members are accessed using dot notation (`.`).

* **`math`** &rarr; See [`stdlib_functions/mathlib.txt`](file:///c:/Users/shrin/OneDrive/Desktop/coding/.vscode/CppProjects/CustomLanguage/stdlib_functions/mathlib.txt) for function reference and examples.
* **`io`** &rarr; See [`stdlib_functions/iolib.txt`](file:///c:/Users/shrin/OneDrive/Desktop/coding/.vscode/CppProjects/CustomLanguage/stdlib_functions/iolib.txt) for file stream reference and examples.

---

##  Architecture & Implementation

1. **Lexer / Tokenizer (`tokenize`)**: Converts source code text into a stream of tokens (`identifier`, `number`, `string_lit`, `symbol`, `keyword`, `eof_token`). Tracks line numbers for detailed error reporting and processes string escape sequences.
2. **AST Nodes (`Expr` & `Stmt`)**: Represents the program structure through object-oriented hierarchy (e.g., `BinaryExpr`, `IfStmt`, `FunctionDefStmt`).
3. **Recursive Descent Parser (`Parser`)**: Parses tokens into executable AST statements.
4. **Environment & Evaluation (`Environment`)**: Evaluates AST nodes recursively maintaining a call stack and lexically scoped variable maps (`scopes`).

---

## Building & Running the Interpreter

### 1. Compilation
Compile using any C++17 compatible compiler (e.g. `g++`):

```bash
g++ -std=c++17 -Wall src/main.cpp src/lexer.cpp src/ast.cpp src/parser.cpp src/stdlib.cpp -o custom_language
```

### 2. Running
Run the executable by passing your `.txt` source file:

```bash
./custom_language your_file.txt
```
