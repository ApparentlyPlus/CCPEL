# CPEL to x86_64 Transpiler

A minimal compiler backend developed in Flex and Bison that compiles a custom prefix expression language (CPEL) directly to native 64-bit x86 assembly (Intel syntax for NASM) and links it via GCC.

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


## The Architecture

- **Frontend**: Flex (`src/lexer.l`) and Bison (`src/parser.y`) perform lexical analysis, AST parsing, syntax analysis and semantic analysis.

- **Symbol Table**: Manages variables and resolves positions as `qword` stack frame offsets.

- **Code Generation**:
  - Direct accumulator-register model (`rax` for integers, `xmm0` for reals).
  - Floating point operations leverage SSE/SSE2 instructions (`addsd`, `subsd`, `mulsd`, `divsd`, `movsd`).
  - Automatic `cvtsi2sd` and `cvttsd2si` instruction emission handles dynamic type coercion.
  - Features immediate-operand checks for NASM, routing literal promotions safely through active registers.

- **IR Instruction Store**: Streamlined linked list (`add_i`, `prt_i`) backed by SGLIB macros, generating clean 64-bit assembly text.

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
