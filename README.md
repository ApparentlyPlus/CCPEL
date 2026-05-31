# CPEL to x86_64 Assembly Compiler

[![CCPEL Tests](https://github.com/ApparentlyPlus/CCPEL/actions/workflows/ci.yml/badge.svg)](https://github.com/ApparentlyPlus/CCPEL/actions/workflows/ci.yml)

A minimal compiler backend developed in Flex and Bison that compiles a custom prefix expression language (CPEL) directly to native 64-bit x86 assembly (Intel syntax for NASM) and links it via GCC.

This was originally a university assignment targeting a JVM backend, but I chose to revisit the project as a fun challenge and build an x86-64 backend instead.

>[!NOTE]
>The acronym CCPEL stands for "Compile CPEL".

## Language Reference

The compiler supports an extensionless prefix syntax:

- Declarations:
  - `(int <var_name>)`: Declares an integer variable.
  - `(float <var_name>)`: Declares a real (double precision) variable.

- Assignments:
  - `(= <var_name> <expr>)`: Assigns an expression. Truncates reals to integers or promotes integers to reals automatically.

- Arithmetic:
  - Prefix operators: `+`, `-`, `*`, `/`. Mixed types are promoted dynamically to double precision real math.

- Side Effects:
  - Pre-increment: `(++ <int_var>)`: Increments variable, then evaluates.
  - Post-increment: `(<int_var> ++)`: Evaluates current value, then increments variable.

- Conditionals:
  - Ternary conditional expressions: `(<relop> <expr> <expr> ? <expr> : <expr>)`
  - Supported relational operators: `>`, `<`, `==` (limited to integer comparisons).

- Output:
  - `(print <expr>)` - Outputs values utilizing C's standard `printf` format strings.

### Examples

Here are some complete, working examples demonstrating CCPEL's prefix syntax:

#### 1. Basic Variable Declarations, Arithmetic, and Printing

```lisp
start basic_program
(int a)
(float b)

(= a 5)
(= b (+ 4.5 (* a 2.0)))

(print a)
(print b)
end
```

This example declares an integer `a` and a float `b`. `a` is assigned `5`, and `b` is assigned `4.5 + (a * 2.0) = 14.50` using prefix operators. Both values are then printed using standard output format strings.

#### 2. Pre and Post Increment Side Effects

```lisp
start increment_demo
(int x)
(int y)
(int z)

(= x 3)
(= y (x++))
(= z (++x))

(print y)
(print z)
(print x)
end
```

Demonstrates how side effects are resolved. `y` is assigned the current value of `x` (`3`) before `x` increments to `4` (post-increment). Then, `x` is incremented to `5` before its value is evaluated and assigned to `z` (`5`) (pre-increment).

#### 3. Ternary Conditionals with Relational Operators

```lisp
start ternary_demo
(int a)
(int b)
(float x)
(float result)

(= a 3)
(= b 7)
(= x 4.5)

(= result (> a b ? (* x 2.0) : (== a 3 ? 15.5 : 42.0)))

(print result)
end
```

Uses nested ternary expressions evaluating directly inside register accumulators. Since `(> a b)` (i.e. `3 > 7`) evaluates to false, the second branch `(== a 3 ? 15.5 : 42.0)` is checked. Since `(== a 3)` is true, `result` is assigned `15.5`.


## The Architecture

- **Frontend**: Flex (`src/lexer.l`) and Bison (`src/parser.y`) perform lexical analysis, parsing, syntax checks, and semantic type analysis in a single pass.

- **Symbol Table**: Manages variables and resolves positions as `qword` stack frame offsets.

- **Code Generation & Evaluation Model**:

  - **Compile Time Stack Construction**: Compiles optimal stack frame frames at compile time. The prologue allocates space exclusively for declared local variables, rounded up to the nearest 16-byte boundary to satisfy System V AMD64 ABI requirements for function calls.

  - **Hardware Stack Expression Evaluation**: Volatile accumulator register contents are dynamically preserved on the CPU's hardware stack via native `push`/`pop` during nested expression evaluations. 

  - **Register Yielding Control Flow**: Statements like ternary conditionals yield values directly inside the accumulator registers (`rax` / `xmm0`) without intermediate memory storage.

  - **SSE Vector Optimization**: Floating-point operations utilize SSE/SSE2 instruction subsets (`addsd`, `subsd`, `mulsd`, `divsd`, `movsd`).
  
  - **Type Promotion**: Automatic `cvtsi2sd` and `cvttsd2si` instructions handle coercion between integers and doubles seamlessly.

- **IR Instruction Store**: Streamlined instruction linked list (`add_i`, `prt_i`) backed by SGLIB macros, serializing clean 64-bit assembly text.

## Build & Test Instructions

### Prerequisites
The build system requires `nasm` and `gcc` installed on a Linux host.

### Build Commands
- **Build Compiler**:

  ```bash
  make
  ```

- **Clean Workspace**:
  Clears compiler builds, intermediate objects, and all generated test assembly/binary artifacts.

  ```bash
  make clean
  ```

### Dynamic Test Suite
Runs incremental compilation and execution for all test cases.

```bash
make test
```

- **Dynamic Discovery**: Scans `tests/` to dynamically discover and execute all valid test programs in alphabetical order.

- **Artifact Segregation**: Preserves intermediate test artifacts in separate directories:
  - `tests/asm/` - Generated assembly source files (`.s`).
  - `tests/obj/` - Assembled object files (`.o`).
  - `tests/bin/` - Linked binary executables.
