# Code Quality Tools Guide

## Overview

This project uses several code quality tools to maintain consistency and catch issues early:

- **clang-format** - Automatic code formatting
- **clang-tidy** - Static analysis and linting
- **pre-commit** - Git hooks for automatic checks

## clang-tidy

### What It Does

`clang-tidy` performs static analysis on C++ code to catch:
- Potential bugs
- Performance issues
- Style violations
- Deprecated patterns
- Modernization opportunities

### Configuration

The `.clang-tidy` file is configured with ESPHome's standard checks and naming conventions:

**Naming Conventions**:
- Classes/Structs/Enums: `CamelCase`
- Functions/Methods: `lower_case`
- Variables: `lower_case`
- Constants: `UPPER_CASE`
- Private members: `lower_case_` (with trailing underscore)
- Private methods: `lower_case_` (with trailing underscore)

**Disabled Checks**:
Many overly strict checks are disabled to match ESPHome's pragmatic style while keeping important safety and bug checks enabled.

### Usage

#### Manual Check

```bash
# Check a single file
clang-tidy components/nibegw/NibeGw.cpp

# Check all files
clang-tidy components/nibegw/*.cpp
```

#### With Compilation Database

For best results, generate a compilation database first:

```bash
# ESPHome generates this during compile
esphome compile examples/test-compile.yaml

# Then run clang-tidy (it will find compile_commands.json)
clang-tidy components/nibegw/NibeGw.cpp
```

#### Fix Issues Automatically

```bash
# Apply automatic fixes (use with caution!)
clang-tidy --fix components/nibegw/NibeGw.cpp
```

#### VS Code Integration

If you have the C/C++ extension installed, clang-tidy runs automatically:

1. Install: **C/C++ Extension** (ms-vscode.cpptools)
2. Install: **clang-tidy** system package
3. Edit settings (already in `.vscode/settings.json`):
   ```json
   "C_Cpp.codeAnalysis.clangTidy.enabled": true
   ```

Warnings will appear as squiggly lines in the editor.

## clang-format

### What It Does

Automatically formats C++ code according to `.clang-format` rules.

### Usage

#### Format Single File

```bash
clang-format -i components/nibegw/NibeGw.cpp
```

#### Format All Files

```bash
clang-format -i components/nibegw/*.cpp components/nibegw/*.h
```

#### Check Without Modifying

```bash
clang-format --dry-run components/nibegw/NibeGw.cpp
```

#### VS Code Integration

Already configured in `.vscode/settings.json`:
- Format on save enabled for C++ files
- Uses clang-format extension

Install: **Clang-Format Extension** (xaver.clang-format)

## pre-commit Hooks

### What It Does

Runs checks automatically before each git commit to catch issues early.

### Setup

```bash
# Install pre-commit
pip install pre-commit

# Install hooks
pre-commit install
```

### Usage

Pre-commit now runs automatically on `git commit`. It will:
1. Run clang-format on staged C++ files
2. Block commit if formatting needed
3. Show what was changed

**Manual run**:
```bash
# Check all files
pre-commit run --all-files

# Check specific file
pre-commit run --files components/nibegw/NibeGw.cpp
```

**Skip hooks** (not recommended):
```bash
git commit --no-verify
```

### Configuration

See `.pre-commit-config.yaml`:
```yaml
repos:
  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v13.0.1
    hooks:
      - id: clang-format
        types_or: [c, c++]
```

## Python Tools

### black (Formatter)

```bash
# Format Python files
black components/nibegw/

# Check without modifying
black --check components/nibegw/
```

### isort (Import Sorter)

```bash
# Sort imports
isort components/nibegw/

# Check only
isort --check-only components/nibegw/
```

### flake8 (Linter)

```bash
# Lint Python files
flake8 components/nibegw/
```

## Complete Workflow

### Before Committing

```bash
# 1. Format C++ code
clang-format -i components/nibegw/*.cpp components/nibegw/*.h

# 2. Check C++ code quality (optional)
clang-tidy components/nibegw/*.cpp

# 3. Format Python code
black components/nibegw/
isort components/nibegw/

# 4. Lint Python code
flake8 components/nibegw/

# 5. Test compilation
esphome compile examples/test-compile.yaml

# 6. Commit (pre-commit will run automatically)
git add .
git commit -m "Your commit message"
```

Or use VS Code tasks:
- `Ctrl+Shift+P` → "Tasks: Run Task" → "Format: clang-format"
- `Ctrl+Shift+P` → "Tasks: Run Task" → "Format: black"

## VS Code Setup Summary

Install these extensions:
1. **C/C++** (ms-vscode.cpptools)
2. **Clang-Format** (xaver.clang-format)
3. **Python** (ms-python.python)
4. **Black Formatter** (ms-python.black-formatter)

Configuration already in `.vscode/settings.json`:
- Format on save (C++ and Python)
- clang-tidy enabled
- Proper tab sizes

## Common Issues

### "clang-tidy: command not found"

Install clang-tidy:
```bash
# Ubuntu/Debian
sudo apt install clang-tidy

# macOS
brew install llvm

# Windows
# Download from LLVM releases
```

### "clang-format: command not found"

Install clang-format:
```bash
# Ubuntu/Debian
sudo apt install clang-format

# macOS
brew install clang-format

# Windows
# Download from LLVM releases
```

### Too Many Warnings

The `.clang-tidy` configuration is already tuned for ESPHome.

To suppress specific warnings in code:
```cpp
// NOLINT(check-name)
int x = 5; // NOLINT(readability-magic-numbers)

// NOLINTNEXTLINE(check-name)
int y = 10;
```

### Pre-commit Blocking Commits

```bash
# Fix formatting issues
clang-format -i <file>

# Or skip (not recommended)
git commit --no-verify
```

## Quality Gates

Before merging/releasing:
1. ✅ All C++ files formatted with clang-format
2. ✅ No clang-tidy warnings (or justified suppressions)
3. ✅ All Python files formatted with black
4. ✅ No flake8 errors
5. ✅ Test compilation succeeds
6. ✅ Pre-commit hooks pass

## Additional Resources

- [clang-tidy docs](https://clang.llvm.org/extra/clang-tidy/)
- [clang-format docs](https://clang.llvm.org/docs/ClangFormat.html)
- [pre-commit docs](https://pre-commit.com/)
- [black docs](https://black.readthedocs.io/)
- [flake8 docs](https://flake8.pycqa.org/)