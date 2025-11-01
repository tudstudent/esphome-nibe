# VS Code IntelliSense Setup

## Understanding the Errors

The IntelliSense errors you see are **harmless** - they occur because VS Code doesn't automatically know where ESP-IDF headers are located. The code **compiles perfectly fine** - these are just IDE warnings.

**Note:** You may see a warning about missing `compile_commands.json` - this is normal! It only exists after the first compilation. Just ignore this warning until you compile once.

## Why This Happens

ESPHome downloads ESP-IDF and toolchains during the first compilation into:
- Windows: `C:\Users\<username>\.platformio\packages\`
- Linux/Mac: `~/.platformio/packages/`

VS Code needs to be told where these are.

## Solution 1: Use the Provided c_cpp_properties.json ✅ RECOMMENDED

We've included `.vscode/c_cpp_properties.json` which tells VS Code where to find headers.

### How to Use It:

1. **First, compile your project once:**
   ```bash
   esphome compile examples/test-compile.yaml
   ```
   This downloads ESP-IDF (~500MB) and creates `compile_commands.json`

2. **Copy the c_cpp_properties.json:**
   ```bash
   # Already included in the repository!
   # Located at: .vscode/c_cpp_properties.json
   ```

3. **Reload VS Code:**
   - Press `Ctrl+Shift+P` (Windows/Linux) or `Cmd+Shift+P` (Mac)
   - Type: "Reload Window"
   - Press Enter

4. **IntelliSense should now work!** ✅

### What It Does:

The `c_cpp_properties.json` tells VS Code:
- ✅ Where ESP-IDF headers are located
- ✅ Which compiler to use (`riscv32-esp-elf-g++`)
- ✅ C++ standard to use (`gnu++17`)
- ✅ Where the compilation database is (`compile_commands.json`)

## Solution 2: Ignore the Warnings (Acceptable)

If you don't care about IntelliSense in C++ files:

1. **The code still compiles perfectly** ✅
2. **No action needed** - Just ignore the red squiggles
3. **Pylance (Python) still works** for `__init__.py` and `climate.py`

## Solution 3: Manual Configuration (Advanced)

If the provided configuration doesn't work, you can manually update paths:

1. **Open:** `.vscode/c_cpp_properties.json`

2. **Update paths to match your system:**
   ```json
   "includePath": [
     "${workspaceFolder}/**",
     "C:/Users/YOUR-USERNAME/.platformio/packages/framework-espidf/components/**",
     "C:/Users/YOUR-USERNAME/.platformio/packages/toolchain-riscv32-esp/riscv32-esp-elf/include/**"
   ]
   ```

3. **Replace `YOUR-USERNAME`** with your actual Windows username

## Troubleshooting

### IntelliSense Still Shows Errors

**Problem:** Red squiggles remain after following Solution 1

**Fixes:**

1. **"Cannot find compile_commands.json" warning:**
   - ✅ **This is NORMAL before first compile**
   - ✅ **Just ignore it** - it will go away after compiling
   - 📝 This file is generated during `esphome compile`
   - ❌ Don't try to create it manually

2. **Verify PlatformIO packages exist:**
   ```bash
   # Windows
   dir %USERPROFILE%\.platformio\packages
   
   # Linux/Mac
   ls ~/.platformio/packages/
   ```
   You should see `framework-espidf` and `toolchain-riscv32-esp`

2. **Check compile_commands.json exists:**
   ```bash
   # After first compile, this should exist:
   # .esphome/build/nibe-test/compile_commands.json
   ```

3. **Rebuild IntelliSense database:**
   - Press `Ctrl+Shift+P`
   - Type: "C/C++: Reset IntelliSense Database"
   - Wait for rebuild (takes 1-2 minutes)

4. **Check VS Code C/C++ extension is installed:**
   - Extension ID: `ms-vscode.cpptools`
   - Should be version 1.17.0 or newer

### ArduinoJson.h Not Found

**This is expected!** We removed Arduino dependencies. The error shows because:
- Header was included in original code
- We removed it in ESP-IDF port
- IntelliSense cache hasn't updated

**Fix:**
1. Reload VS Code window
2. Or rebuild IntelliSense database (see above)

### sys/ioctl.h Not Found

**This is normal!** This header is part of ESP-IDF, not standard Windows includes.

**Fix:**
- Make sure `c_cpp_properties.json` is in place
- Reload VS Code after first compilation

## What Files Control IntelliSense?

### .vscode/c_cpp_properties.json ✅ IN REPOSITORY
Tells VS Code where to find headers and which compiler to use.

### .vscode/.browse.VC.db ❌ NOT IN REPOSITORY
IntelliSense cache database. Excluded via `.gitignore` (user-specific).

### .vscode/ipch/ ❌ NOT IN REPOSITORY
IntelliSense pre-compiled header cache. Excluded via `.gitignore` (user-specific).

### .esphome/build/*/compile_commands.json ❌ NOT IN REPOSITORY
Generated during compilation. Excluded via `.gitignore` (build artifact).

## Summary

| Error | Impact | Solution |
|-------|--------|----------|
| `#include errors detected` | ❌ IDE only | Use `c_cpp_properties.json` |
| `cannot open source file` | ❌ IDE only | Compile once, reload VS Code |
| Compilation fails | 🔴 REAL PROBLEM | Fix the actual error |

**Key Point:** If your code **compiles successfully**, IntelliSense errors are **cosmetic only**.

## For GitHub Users

When others clone your repository:

1. ✅ They get `c_cpp_properties.json` (in repo)
2. ❌ They don't get `.esphome/` (excluded)
3. ❌ They don't get IntelliSense cache (excluded)
4. ✅ They compile once → IntelliSense works

This is the correct setup! 🎯

## Quick Fix Commands

```bash
# 1. Compile to generate all necessary files
esphome compile examples/test-compile.yaml

# 2. Reload VS Code
# Press Ctrl+Shift+P → "Reload Window"

# 3. If still having issues, reset IntelliSense
# Press Ctrl+Shift+P → "C/C++: Reset IntelliSense Database"
```

## Expected Behavior

After following Solution 1:

- ✅ No red squiggles in C++ files
- ✅ Auto-complete works
- ✅ Go to definition works
- ✅ Hover shows documentation
- ✅ Compilation still works

## Still Having Issues?

If IntelliSense still doesn't work after trying all solutions:

1. **It's OK!** The code compiles fine without it
2. **Alternative:** Use a simpler editor (Notepad++, nano, etc.)
3. **Report:** Open an issue on GitHub with your VS Code version and OS

---

**Bottom Line:** IntelliSense errors are annoying but harmless. If it compiles, it works! ✅