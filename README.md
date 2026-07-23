# Custom Language

A custom scripting language interpreter implemented in C++. It parses source code using a recursive descent parser into an Abstract Syntax Tree composed of C++ AST node objects (`Expr` and `Stmt`), which are then dynamically evaluated in a scoped environment.

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
```

---

### 4. Operators & Expressions
Supported arithmetic, comparison, logical, and string operations with standard operator precedence:

* **Arithmetic Operators**: `+`, `-`, `*`, `/`, unary `-`
  * String concatenation is automatically supported with `+` if either operand is a string (e.g., `"Value: " + 10`).
  * Division by zero returns `0.0` safely.
* **Comparison Operators**: `>`, `<`, `>=`, `<=`, `==`, `!=`
* **Parentheses**: `(` and `)` for overriding operator precedence.

---

### 5. Control Flow Statements

#### `if` / `else` Statements
Conditional execution based on condition evaluation.
```custom
if (x > 10) {
    print("Greater")
} else {
    print("Smaller or Equal")
}
```

#### `while` Loops
Loops as long as the condition evaluates to `true`.
```custom
int count = 0
while (count < 5) {
    print(count)
    count = count + 1
}
```

#### `for` Loops
Standard 3-part `for` loop syntax with comma separators: `for (initializer, condition, increment) { ... }`
```custom
for (int i = 0, i < 5, i = i + 1) {
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

---

### 7. Built-in Functions

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

---

### 8. Comments & Syntax Rules
* **No Semicolons**: Semicolons (`;`) are strictly prohibited and will trigger a compiler error if present.
* **Single-line Comments**: Start with `//` and ignore all subsequent characters on that line.

```custom
// This is a single-line comment
int x = 5 // Inline comment
```

---

##  Architecture & Implementation

1. **Lexer / Tokenizer (`tokenize`)**: Converts source code text into a stream of tokens (`identifier`, `number`, `string_lit`, `symbol`, `keyword`, `eof_token`). Tracks line numbers for detailed error reporting and processes string escape sequences.
2. **AST Nodes (`Expr` & `Stmt`)**: Represents the program structure through object-oriented hierarchy (e.g., `BinaryExpr`, `IfStmt`, `FunctionDefStmt`).
3. **Recursive Descent Parser (`Parser`)**: Parses tokens into executable AST statements.
4. **Environment & Evaluation (`Environment`)**: Evaluates AST nodes recursively maintaining a call stack and lexically scoped variable maps (`scopes`).

---

## Running the Interpreter

Compile using any C++17 compatible compiler:

```bash
g++ -std=c++17 custom_language.cpp -o custom_language
```

### Passing script file as command-line argument:
```bash
./custom_language example.txt
```

### Default prompt:
If no argument is passed, the program prompts for the file path:
```bash
./custom_language
Enter your file path for cus language here (.txt): example.txt
```