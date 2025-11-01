# Development Setup Guide

## Prerequisites

- Python 3.8 or later
- Git
- (Optional) clang-format for C++ formatting
- (Optional) VS Code for IDE support

## Initial Setup

### 1. Clone Repository

```bash
git clone https://github.com/yourusername/esphome-nibe-idf.git
cd esphome-nibe-idf
```

### 2. Create Virtual Environment

```bash
# Create venv
python -m venv .venv

# Activate (Linux/macOS)
source .venv/bin/activate

# Activate (Windows)
.venv\Scripts\activate
```

### 3. Install ESPHome

```bash
pip install esphome
```

### 4. (Optional) Install Development Tools

```bash
# Python formatting and linting
pip install black isort flake8

# Pre-commit hooks
pip install pre-commit
pre-commit install
```

## Project Structure

```
esphome-nibe-idf/
├── components/
│   └── nibegw/              # ESPHome component
│       ├── *.cpp, *.h       # C++ implementation
│       └── *.py             # Python config
├── examples/                # Example YAML configs
│   ├── test-compile.yaml    # For testing
│   ├── minimal.yaml         # Minimal config
│   └── complete.yaml        # Full featured
├── docs/                    # Documentation
├── .vscode/                 # VS Code config
├── .clang-format           # C++ formatting
├── .editorconfig           # Editor settings
├── .gitattributes          # Git line endings
├── .gitignore              # Git exclusions
└── README.md               # Main docs
```

## Development Workflow

### Test Compilation

```bash
# Activate venv if not already active
source .venv/bin/activate  # or .venv\Scripts\activate on Windows

# Compile test configuration
esphome compile examples/test-compile.yaml
```

### Code Formatting

#### C++ Files

```bash
# Format all C++ files
clang-format -i components/nibegw/*.cpp components/nibegw/*.h

# Or check without modifying
clang-format --dry-run components/nibegw/*.cpp
```

#### Python Files

```bash
# Format Python code
black components/nibegw/

# Sort imports
isort components/nibegw/

# Lint
flake8 components/nibegw/
```

### Making Changes

1. **Edit files** in `components/nibegw/`
2. **Format code** using clang-format/black
3. **Test compile** using `examples/test-compile.yaml`
4. **Commit changes** with descriptive message
5. **Push to GitHub**

### VS Code Tasks

If using VS Code, press `Ctrl+Shift+P` → "Tasks: Run Task" to access:

- **ESPHome: Compile Test Config** - Build test configuration
- **ESPHome: Clean Build** - Clean build directory
- **ESPHome: Show Logs** - View device logs
- **Format: clang-format** - Format all C++ files
- **Format: black** - Format Python files
- **Setup: Create venv** - Create virtual environment
- **Setup: Install ESPHome** - Install ESPHome in venv

Or use keyboard shortcuts:
- `Ctrl+Shift+B` - Build (compile test config)

## Testing

### 1. Test Compilation

```bash
esphome compile examples/test-compile.yaml
```

This verifies:
- Code compiles successfully
- No syntax errors
- Dependencies resolve correctly
- Framework compatibility

### 2. Clean Build

```bash
esphome clean examples/test-compile.yaml
esphome compile examples/test-compile.yaml
```

Forces complete rebuild from scratch.

### 3. Flash to Device

```bash
# First time (USB)
esphome upload examples/test-compile.yaml

# OTA (after first flash)
esphome upload examples/test-compile.yaml --device 192.168.0.34
```

### 4. Monitor Logs

```bash
esphome logs examples/test-compile.yaml
```

Or specify device:
```bash
esphome logs examples/test-compile.yaml --device 192.168.0.34
```

## Common Issues

### Import Errors

**Problem**: `ModuleNotFoundError: No module named 'esphome'`

**Solution**: Activate virtual environment
```bash
source .venv/bin/activate  # or .venv\Scripts\activate
```

### Compilation Errors

**Problem**: `AsyncUDP.h not found`

**Solution**: Ensure `framework: type: esp-idf` (not Arduino)

**Problem**: `Socket errors`

**Solution**: Check all includes in NibeGwComponent.h:
```cpp
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <fcntl.h>
```

### Path Issues

**Problem**: ESPHome can't find component

**Solution**: Use correct path in YAML:
```yaml
external_components:
  - source:
      type: local
      path: components  # Relative to YAML file
    components: [nibegw]
    refresh: 0s
```

## File-Specific Notes

### C++ Files (`*.cpp`, `*.h`)

- Use 2-space indentation
- Format with clang-format before commit
- Follow ESPHome coding style
- Keep lines under 120 characters

### Python Files (`*.py`)

- Use 4-space indentation
- Format with black
- Sort imports with isort
- Follow PEP 8

### YAML Files (`*.yaml`)

- Use 2-space indentation
- No tabs
- Validate syntax before commit

## Git Workflow

### Before Committing

```bash
# Format code
clang-format -i components/nibegw/*.cpp components/nibegw/*.h
black components/nibegw/
isort components/nibegw/

# Test compilation
esphome compile examples/test-compile.yaml

# Stage changes
git add components/nibegw/

# Commit
git commit -m "Description of changes"
```

### Branch Strategy

```bash
# Create feature branch
git checkout -b feature/your-feature-name

# Make changes and commit
git add .
git commit -m "Add new feature"

# Push to GitHub
git push origin feature/your-feature-name

# Create pull request on GitHub
```

## Debugging Tips

### Enable Verbose Logging

Edit your test YAML:
```yaml
logger:
  level: VERBOSE
  logs:
    nibegw: VERBOSE
```

### Use Serial Monitor

```bash
esphome logs examples/test-compile.yaml
```

### Check UDP Traffic

On your monitoring server:
```bash
tcpdump -i eth0 -n udp port 9999 or udp port 10000
```

### Network Issues

```bash
# On ESP32, check IP
# In logs, look for:
# [ethernet:123]: Network online (192.168.x.x)

# Ping test
ping 192.168.0.34
```

## IDE Configuration

### VS Code Recommended Extensions

- **C/C++** (ms-vscode.cpptools)
- **Python** (ms-python.python)
- **ESPHome** (esphome.esphome-vscode)
- **YAML** (redhat.vscode-yaml)
- **Clang-Format** (xaver.clang-format)

### VS Code Settings

Already configured in `.vscode/settings.json`:
- Python interpreter points to `.venv`
- C++ standard is C++11
- Auto-format on save for C++ and Python
- Proper tab sizes (2 for C++, 4 for Python)

## Performance Testing

### Memory Usage

Add to test YAML:
```yaml
sensor:
  - platform: template
    name: "Free Heap"
    lambda: return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    update_interval: 10s
```

### CPU Usage

Monitor loop execution time in logs:
```
[nibegw:123]: Loop time: 15ms
```

Should be <30ms typically.

## Release Process

### 1. Version Bump

Update version in:
- README.md
- Component code (if applicable)
- CHANGELOG.md

### 2. Test Everything

```bash
# Clean build
esphome clean examples/test-compile.yaml
esphome compile examples/test-compile.yaml

# Test all examples
esphome compile examples/minimal.yaml
esphome compile examples/complete.yaml
```

### 3. Update Documentation

- README.md
- CHANGELOG.md
- Migration guides if needed

### 4. Create Release

```bash
git tag -a v2.0.0 -m "Release v2.0.0"
git push origin v2.0.0
```

### 5. Create GitHub Release

- Go to GitHub Releases
- Create new release from tag
- Add changelog
- Upload artifacts if needed

## Getting Help

- **Documentation**: See `docs/` folder
- **Issues**: [GitHub Issues](https://github.com/yourusername/esphome-nibe-idf/issues)
- **ESPHome**: [Discord](https://discord.gg/KhAMKrd)

## Next Steps

1. Read [ARCHITECTURE.md](ARCHITECTURE.md) to understand system design
2. Review existing code in `components/nibegw/`
3. Try compiling `examples/test-compile.yaml`
4. Make your first change and test it
5. Submit a pull request!