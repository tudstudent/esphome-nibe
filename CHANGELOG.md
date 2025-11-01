# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0-espidf] - 2024-11-01

### Added - Major ESP-IDF Port

#### Core Framework
- Complete migration from Arduino framework to ESP-IDF
- Native ESP-IDF UART driver implementation
- ESP-IDF GPIO handling
- Standard C++ type system (no Arduino dependencies)

#### Networking
- **BSD sockets implementation** replacing AsyncUDP
- Native lwIP integration
- Proper socket lifecycle management
- Non-blocking socket operations
- Better error handling and recovery
- IPv4 address handling via `inet_pton()`

#### Ethernet Support
- **SPI Ethernet support**
  - DM9051 (tested configuration)
  - W5500 (should work)
  - KSZ8851SNL (should work)
- **RMII Ethernet support**
  - LAN8720
  - RTL8201
  - IP101
  - DP83848
  - KSZ8041
- Manual IP configuration
- DHCP support via ESPHome

#### Documentation
- START_HERE.md - Quick start guide
- SETUP_INSTRUCTIONS.md - Detailed setup
- DEVELOPMENT.md - Development workflow
- CODE_QUALITY.md - Quality tools guide
- docs/ARCHITECTURE.md - System architecture (16KB, with diagrams)
- docs/MIGRATION_SUMMARY.md - Arduino to ESP-IDF migration guide
- docs/QUICK_REFERENCE.md - Code patterns and examples
- docs/TESTING_CHECKLIST.md - Comprehensive testing procedures
- Multiple example configurations

#### Development Tools
- `.clang-format` - C++ code formatting
- `.clang-tidy` - Static analysis configuration
- `.editorconfig` - Editor settings
- `.pre-commit-config.yaml` - Git hooks
- `.gitignore` - Proper exclusions
- `.gitattributes` - Line ending rules
- VS Code configuration
  - settings.json - IDE settings
  - tasks.json - Build tasks

### Changed - Breaking Changes

#### Framework
- **BREAKING:** Requires `framework: type: esp-idf` in YAML
- **BREAKING:** No longer compatible with Arduino framework
- **BREAKING:** Different build flags and dependencies

#### Code Structure
- Replaced all `byte` → `uint8_t`
- Replaced all `boolean` → `bool`
- Removed all `Arduino.h` includes
- Changed UART implementation to ESP-IDF driver
- Changed GPIO handling to ESP-IDF API

#### Networking
- **BREAKING:** Complete UDP implementation rewrite
- Removed AsyncUDP dependency
- Added BSD socket layer
- Changed IP address handling
- New connection state management

#### Configuration
- **BREAKING:** External component source must be updated
- No changes to YAML configuration format (compatible)
- Same nibegw configuration options
- Same protocol implementation

### Fixed

#### Type Safety
- Fixed all Pylance type checking errors
- Proper type annotations in Python code
- Correct enum handling (Addresses, Token)
- Fixed Optional field handling in schemas

#### Compilation
- Fixed `isnan()` calls (added `std::` prefix)
- Fixed IPAddress construction from uint32_t
- Fixed static_cast issues with IPAddress
- Proper inet_pton() usage for IP conversion
- Added missing includes

#### Code Quality
- Removed C-style casts
- Proper const correctness
- Better error handling
- Memory leak prevention
- Thread-safe operations

### Technical Details

#### Lines of Code
- C++ code: ~1,450 lines (complete rewrite of networking)
- Python code: ~250 lines (updated for type safety)
- Documentation: ~50 pages
- Configuration: 7 dotfiles

#### Files Changed
- NibeGw.cpp - UART handling, ESP-IDF driver
- NibeGw.h - Type definitions, removed Arduino types
- NibeGwComponent.cpp - Complete networking rewrite
- NibeGwComponent.h - BSD socket declarations
- NibeGwClimate.cpp - Fixed isnan() calls
- NibeGwClimate.h - No Arduino dependencies
- __init__.py - Type safety, enum handling
- climate.py - No changes needed

#### Build System
- Removed AsyncUDP library dependency
- Added ESP-IDF framework requirement
- Updated compiler flags
- Changed build configuration

### Testing

#### Verified
- ✅ Compiles successfully with ESP-IDF
- ✅ ESPHome 2024.10.3
- ✅ ESP32-C3-DevKitM-1
- ✅ DM9051 Ethernet configuration
- ✅ WiFi configuration
- ✅ All YAML examples
- ✅ Python type checking (Pylance)
- ✅ C++ compilation

#### Pending
- ⚠️ Real hardware testing with Nibe heat pump
- ⚠️ Ethernet module physical testing
- ⚠️ Long-term stability testing
- ⚠️ Performance benchmarking

### Migration Guide

To migrate from Arduino version:

1. **Change framework** in your YAML:
   ```yaml
   esp32:
     framework:
       type: esp-idf  # Changed from arduino
   ```

2. **Update component source**:
   ```yaml
   external_components:
     - source:
         type: git
         url: https://github.com/YOUR-USERNAME/esphome-nibe-espidf.git
       components: [ nibegw ]
   ```

3. **Test WiFi first** before adding Ethernet

4. **Verify** all heat pump functionality

5. **Keep backup** of working Arduino configuration

### Known Issues

1. **First compilation slow** - ESP-IDF toolchain download (~500MB)
2. **VS Code warnings** - IntelliSense can't find ESP-IDF headers (compile works fine)
3. **Limited testing** - Awaiting real-world hardware verification
4. **No auto-migration** - Manual configuration changes required

### Credits

- **Original**: [elupus/esphome-nibe](https://github.com/elupus/esphome-nibe)
- **ESP-IDF Port**: Complete refactoring for ESP-IDF and Ethernet support
- **Testing**: Community contributors (thank you!)

---

## Original Version (Arduino-based)

For the original Arduino-based version, see:
https://github.com/elupus/esphome-nibe

This ESP-IDF port maintains protocol compatibility while completely changing the underlying implementation.