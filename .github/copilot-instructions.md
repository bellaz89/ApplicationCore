# Copilot Instructions for ChimeraTK ApplicationCore

This document describes key information about the ChimeraTK ApplicationCore project to help AI assistants work effectively in this codebase.

## Project Overview

**ChimeraTK ApplicationCore** (v04.07.01) is a C++ framework for building distributed real-time control system applications. The framework enables developers to create modular applications where:

- Each module runs in its own thread and declares typed process variables (inputs/outputs)
- Variables are wired together automatically based on their paths
- The framework handles scheduling, data validity propagation, and version tracking
- All variables are automatically published to control system adapters (EPICS, DOOCS, OPC-UA, etc.)
- Applications can be scripted with Lua (via sol2) or Python (via pybind11)

The codebase is part of the ChimeraTK ecosystem and uses the project-template merged into the repository for build infrastructure.

## Build and Test Commands

### Prerequisites

Install required packages:
```bash
# Core dependencies
sudo apt-get install cmake libboost-dev libxml++2.6-dev

# For Lua support
sudo apt-get install luajit  # or: lua, lua-dev

# For Python support
sudo apt-get install pybind11 python3-dev mypy  # mypy provides stubgen

# ChimeraTK packages (may need to add DESY repository)
# See: https://github.com/ChimeraTK/ApplicationCore
```

### Building the Library

```bash
cd build
cmake ..
make
```

Build options:
```bash
cmake -DBUILD_TESTS=ON ..          # Include tests (default: ON)
cmake -DENABLE_LUA_BINDINGS=ON ..  # Enable Lua scripting (default: ON)
cmake -DLUA_IMPL=AUTO ..           # LuaJIT first, then Lua (default: AUTO, also: LUAJIT, LUAC)
```

### Running All Tests

```bash
cd build
ctest
```

Or with verbose output:
```bash
ctest --verbose
```

### Running a Single Test

Each test is a compiled executable. Example:

```bash
cd build
# Run individual test
./testApplication
./testDeviceAccessors
```

List all available tests:
```bash
ctest --show-only
```

### Code Style Check

The project uses clang-format for code style (clang-format-14 preferred). Before committing:

```bash
# Check style (dry run)
clang-format -i include/*.h src/*.cc  # This modifies files, so be careful
```

The `.clang-format` file defines the ChimeraTK style (LLVM-based with specific customizations).

### Building Documentation

Doxygen documentation can be generated (if enabled):
```bash
cd build
cmake --build . --target doxygen  # if available
```

## Architecture and Key Components

### Core Module Architecture

**Application** — Root container that manages the module hierarchy
- Subclass `Application` and add modules to its constructor
- Calls `initialise()` during startup to wire variables and start threads

**ApplicationModule** — Base class for all modules
- Override `mainLoop()` to implement module logic
- Declare process variables using accessor objects
- Runs in its own thread, wakes on new data arrival

**Accessors** — Type-safe interfaces to process variables
- **ScalarAccessor<T>** — Single scalar values
- **ArrayAccessor<T>** — Arrays of values
- **VoidAccessor** — Trigger-only variables
- **ConsumingFanOut** / **FeedingFanOut** — Fan-out patterns
- **TriggerFanOut** — Conditional triggering

### Variable Wiring

Variables connect automatically by path matching:
- Paths follow hierarchical naming: `/ModuleA/variable`, `/ModuleB/variable`
- An output accessor produces data that input accessors consume
- The framework handles buffering and synchronization

### Module Groups and Hierarchy

**ModuleGroup** — Container for related modules
- Enables logical grouping without threading overhead
- Modules in a group run in the parent's thread
- Reduces threading complexity for tightly-coupled components

**VariableGroup** — Semantically groups variables
- Used in configuration and for organizing related inputs/outputs
- Not related to threading or wiring

### Threading and Scheduling

- Each ApplicationModule runs in its own thread
- Modules are scheduled event-driven: they wake only when new data arrives on any input
- ModuleGroup modules run in parent's thread (no additional threads)
- Thread safety is managed by the framework

### Data Validity and Versioning

- The framework automatically propagates:
  - **DataValidity** flags (indicates if data is valid/stale)
  - **Version numbers** (incremented on each update)
  - These metadata are transparent to module logic but available when needed

### Exception Handling

- **ExceptionHandlingDecorator** — Wraps accessors to handle module exceptions
- Can recover from errors or propagate exceptions to outputs
- Supports both synchronous and recovery-based error handling

### Device Integration

**DeviceManager** and **DeviceModule** — Hardware device integration
- `DeviceManager` provides access to ChimeraTK devices
- `DeviceModule` manages device I/O in a dedicated thread
- Device paths are mapped through the application configuration

## Key Conventions and Patterns

### Module Naming and Organization

- Module class names typically match their role: `TemperatureController`, `DataLogger`, etc.
- Header files in `include/`, implementation in `src/`
- One class per file (mostly)

### Variable Path Naming

Paths follow the module hierarchy:
```cpp
ScalarAccessor<float> temperature{this, "/TemperatureModule/temperature", AccessModeFlags::R};
ScalarAccessor<float> setpoint{this, "/ControlModule/setpoint", AccessModeFlags::W};
```

Path components should be descriptive and use PascalCase for module names, snake_case for variable names (convention).

### Configuration Files (XML)

- Stored as `<AppName>-config.xml`
- Defines module instances, connections, and initial values
- Read by `ConfigReader` during `initialise()`
- Example structure:
  ```xml
  <application>
    <modules>
      <module name="TempMod" type="TemperatureController"/>
    </modules>
  </application>
  ```

### Testing Pattern with TestFacility

**TestFacility** is the primary testing interface:
- Allows writing and reading variables from tests
- Enables stepping module execution
- Provides access to all application variables
- Example:
  ```cpp
  auto tf = app.getTestFacility();
  tf.write("/Module/var", value);
  tf.stepApplication();
  auto result = tf.read<int>("/Module/output");
  ```

### Lua and Python Scripting Modules

- **LuaModule** — Base class for Lua-based modules using sol2
- **Python binding** — Experimental; uses pybind11 for Python module support
- Scripted modules interface the same as C++ modules
- Configuration specifies which modules are scripted vs compiled

### Internal Modules

**InternalModule** — Used for framework-internal modules (rarely subclassed by users)
- Already handles scheduling and threading
- Used by the framework for special purposes

### Circular Dependency Detection

The framework includes built-in circular dependency detection:
- Detects problematic wiring patterns
- `CircularDependencyDetector` — Analyzes the variable network
- Helps identify design issues early

## File Structure

```
ApplicationCore/
├── include/               # Public C++ headers
│   ├── Application.h      # Application base class
│   ├── ApplicationModule.h # Module base class
│   ├── Accessor*.h        # Accessor types (Scalar, Array, Void, FanOut, etc.)
│   └── ...
├── src/                   # C++ implementation
├── Python/                # Python binding source
│   └── bindings/          # pybind11 binding code
├── Lua/                   # Lua binding stubs and documentation
├── tests/                 # Test suite
│   ├── executables_src/   # Test implementations
│   ├── include/           # Test headers
│   └── config/            # Test configuration files
├── doc/                   # Doxygen documentation source
├── cmake/                 # Build system utilities (from project-template)
└── CMakeLists.txt         # Main build configuration
```

## Important Build Configuration Notes

- **C++ Standard**: C++17 minimum
- **Dependencies**: The project uses `ChimeraTK::ApplicationCore` imported target which automatically includes all transitive dependencies
- **Python Support**: Conditional; builds Python bindings if pybind11 >= 2.10 is found; generates `.pyi` stubs for IDE support
- **Lua Support**: Optional; searches for LuaJIT first, then Lua; uses sol2 library for bindings
- **ASAN/TSAN builds**: Python stub generation is skipped to avoid symbol resolution errors

## Workflow Tips

### When Adding a New Module

1. Create header file with `ApplicationModule` subclass
2. Declare accessors as member variables
3. Implement `mainLoop()` 
4. Add to application in main `Application` subclass constructor
5. Update configuration XML if needed
6. Add tests in `tests/executables_src/testNewModule.cc`

### When Debugging Variable Wiring

- Use `VariableNetworkNodeDumpingVisitor` to trace connections
- Check that paths match exactly (case-sensitive)
- Verify AccessModeFlags (read vs write)
- Use TestFacility to inject test values and verify outputs

### When Adding Framework Tests

- Tests use simple main() execution (not Boost Unit Test)
- Each test is a separate executable built by CMakeLists
- Manual tests go in `tests/executables_src/manual/`
- Config files copied to build directory automatically
