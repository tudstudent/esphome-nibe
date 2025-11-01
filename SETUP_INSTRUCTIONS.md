# Setup Instructions & File Organization

## ✅ Files Ready to Use (Keep These)

### Development Configuration
- ✅ `.clang-format` - C++ code formatting rules (ESPHome style)
- ✅ `.editorconfig` - Editor settings (updated for C++/Python)
- ✅ `.gitattributes` - Git line ending rules (good as-is)
- ✅ `.gitignore` - Git exclusions (created - includes .venv, .esphome, etc.)
- ✅ `.pre-commit-config.yaml` - Pre-commit hooks for clang-format

### VS Code Configuration
- ✅ `.vscode/settings.json` - Updated for cross-platform, Python + C++
- ✅ `.vscode/tasks.json` - ESPHome build tasks, formatting tasks

### Documentation
- ✅ `README.md` - Updated for ESP-IDF, complete usage guide
- ✅ `DEVELOPMENT.md` - Development workflow and setup guide
- ✅ `ARCHITECTURE.md` - System architecture (from previous work)
- ✅ `MIGRATION_SUMMARY.md` - Arduino → ESP-IDF changes (from previous work)
- ✅ `QUICK_REFERENCE.md` - Code patterns (from previous work)
- ✅ `TESTING_CHECKLIST.md` - Test procedures (from previous work)
- ✅ `INDEX.md` - Navigation guide (from previous work)

### Example Configurations
- ✅ `examples/test-compile.yaml` - For initial testing (your exact setup)
- ✅ `examples/minimal.yaml` - Minimal working config
- ✅ `examples/complete.yaml` - Full featured with RMU40

### Source Code (from previous work)
- ✅ `components/nibegw/NibeGw.cpp`
- ✅ `components/nibegw/NibeGw.h`
- ✅ `components/nibegw/NibeGwComponent.cpp`
- ✅ `components/nibegw/NibeGwComponent.h`
- ✅ `components/nibegw/NibeGwClimate.cpp`
- ✅ `components/nibegw/NibeGwClimate.h`
- ✅ `components/nibegw/__init__.py`
- ✅ `components/nibegw/climate.py`

## ❌ Files to Remove

### Old VS Code Files (Replaced)
- ❌ `settings.json` (upload) - Replaced with updated version in `.vscode/`
- ❌ `tasks.json` (upload) - Replaced with ESPHome-specific version in `.vscode/`

### Old Documentation
- ❌ `README.md` (upload) - Replaced with ESP-IDF version
- ❌ `_editorconfig` (upload) - Replaced with updated `.editorconfig`

**Note**: The uploaded files with `_` prefix were just renamed versions. We've properly renamed and updated them.

## 📁 Correct Directory Structure

```
esphome-nibe-idf/                    # Your repository root
│
├── .vscode/                          # VS Code configuration
│   ├── settings.json                 # ✅ Use new version
│   └── tasks.json                    # ✅ Use new version
│
├── components/                       # ESPHome components
│   └── nibegw/                       # Main component
│       ├── NibeGw.cpp               # ✅ ESP-IDF version
│       ├── NibeGw.h                 # ✅ ESP-IDF version
│       ├── NibeGwComponent.cpp      # ✅ ESP-IDF version (BSD sockets)
│       ├── NibeGwComponent.h        # ✅ ESP-IDF version (BSD sockets)
│       ├── NibeGwClimate.cpp        # ✅ ESP-IDF version
│       ├── NibeGwClimate.h          # ✅ ESP-IDF version
│       ├── __init__.py              # ✅ ESP-IDF version (no AsyncUDP)
│       └── climate.py               # ✅ No changes
│
├── docs/                             # Documentation
│   ├── ARCHITECTURE.md              # ✅ System design
│   ├── MIGRATION_SUMMARY.md         # ✅ Change details
│   ├── QUICK_REFERENCE.md           # ✅ Code patterns
│   ├── TESTING_CHECKLIST.md         # ✅ Test guide
│   └── INDEX.md                     # ✅ Navigation
│
├── examples/                         # Example configs
│   ├── test-compile.yaml            # ✅ For testing (your setup)
│   ├── minimal.yaml                 # ✅ Minimal config
│   └── complete.yaml                # ✅ Full config with RMU40
│
├── .clang-format                    # ✅ C++ formatting
├── .editorconfig                    # ✅ Editor config (updated)
├── .gitattributes                   # ✅ Git line endings
├── .gitignore                       # ✅ Git exclusions (new)
├── .pre-commit-config.yaml          # ✅ Pre-commit hooks
├── README.md                        # ✅ Main docs (ESP-IDF version)
└── DEVELOPMENT.md                   # ✅ Dev guide (new)
```

## 🚀 Next Steps

### 1. Organize Your Files

```bash
# In your project directory, organize like this:
mkdir -p .vscode
mkdir -p components/nibegw
mkdir -p docs
mkdir -p examples

# Copy files to correct locations (use the outputs from earlier)
```

### 2. Create Python Virtual Environment

```bash
# Create venv
python -m venv .venv

# Activate
source .venv/bin/activate  # Linux/macOS
# or
.venv\Scripts\activate  # Windows

# Install ESPHome
pip install esphome
```

### 3. Test Compilation

```bash
# Make sure you're in the project root and venv is activated
esphome compile examples/test-compile.yaml
```

**Expected output**:
- Should compile successfully
- No errors about AsyncUDP
- No errors about Arduino.h
- Build completes with ESP-IDF framework

### 4. Customize test-compile.yaml

Edit `examples/test-compile.yaml` to match your setup:

```yaml
ethernet:
  manual_ip:
    static_ip: 192.168.0.34  # ← Your ESP32 IP
    gateway: 192.168.0.1     # ← Your router IP

nibegw:
  udp:
    target:
      - ip: 192.168.0.4      # ← Your monitoring server IP
    source:
      - 192.168.0.4          # ← Your monitoring server IP
```

### 5. First Flash (if you have hardware)

```bash
# Connect ESP32-C3 via USB
esphome upload examples/test-compile.yaml

# Monitor logs
esphome logs examples/test-compile.yaml
```

## 🔍 What Changed from Arduino Version

### Removed Dependencies
- ❌ `AsyncUDP` library
- ❌ `Arduino.h` header
- ❌ WiFi-specific includes
- ❌ Arduino types (byte, boolean)

### Added for ESP-IDF
- ✅ `lwip/sockets.h` - BSD sockets
- ✅ `fcntl.h` - Non-blocking sockets
- ✅ Standard C++ types (uint8_t, bool)

### Code Changes
- `NibeGwComponent.cpp`: Complete UDP rewrite using BSD sockets
- All files: `byte` → `uint8_t`, `boolean` → `bool`
- `__init__.py`: Removed AsyncUDP library additions

### Configuration Changes
- Framework: `type: esp-idf` (was Arduino)
- Network: `ethernet:` (was `wifi:`)
- Everything else: Identical syntax

## 📝 Checklist Before First Compile

- [ ] Virtual environment created (`.venv/`)
- [ ] ESPHome installed (`pip install esphome`)
- [ ] All files in correct directory structure
- [ ] `examples/test-compile.yaml` customized with your IPs
- [ ] Pin numbers match your hardware
- [ ] Framework set to `esp-idf` in YAML

## ⚠️ Important Notes

### About .gitignore
The new `.gitignore` includes:
- `.venv/` - Your Python virtual environment (don't commit this)
- `.esphome/` - ESPHome build cache
- `secrets.yaml` - If you use secrets (recommended)

### About .vscode Folder
You asked if the .vscode folder needs rework - **Yes, I've provided updated versions**:
- `settings.json` - Works cross-platform (Windows/Linux/macOS)
- `tasks.json` - Has ESPHome-specific build tasks

The old versions you had were too simple and Windows-specific.

### About Pin Numbers
The `examples/test-compile.yaml` uses these pins:
- **Ethernet (DM9051)**: GPIO7, 10, 3, 9, 8, 6
- **RS-485**: GPIO20, 21, 19

**Make sure these match your actual wiring!**

## 🐛 Troubleshooting

### "ESPHome not found"
```bash
# Make sure venv is activated
source .venv/bin/activate  # or .venv\Scripts\activate

# Install ESPHome
pip install esphome
```

### "Component nibegw not found"
```yaml
# In your YAML, use correct path:
external_components:
  - source:
      type: local
      path: components  # Must be relative to YAML file
    components: [nibegw]
    refresh: 0s
```

### "AsyncUDP.h not found"
- Check framework is `type: esp-idf` (not Arduino)
- Verify you're using the ESP-IDF version of files

### Compilation takes forever
- Normal on first compile (downloads ESP-IDF toolchain)
- Subsequent compiles are much faster
- Clean build if issues: `esphome clean examples/test-compile.yaml`

## 📞 Need Help?

1. Check `DEVELOPMENT.md` for workflow
2. Check `TESTING_CHECKLIST.md` for validation
3. Check `docs/` folder for detailed explanations
4. Enable verbose logging in YAML:
   ```yaml
   logger:
     level: VERBOSE
     logs:
       nibegw: VERBOSE
   ```

## ✨ Ready to Go!

Once you have the correct directory structure and files in place:

```bash
# Activate venv
source .venv/bin/activate

# Test compile
esphome compile examples/test-compile.yaml

# If successful, you're ready to flash!
esphome upload examples/test-compile.yaml
```

Good luck! 🎉