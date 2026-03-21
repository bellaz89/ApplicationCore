# Application Configuration File (`-config.xml`)

ChimeraTK ApplicationCore supports an XML configuration file that is loaded automatically at application start-up.  The file provides runtime-configurable values to your modules and declares which scripted modules (Python, Lua) to load.

---

## File naming and location

The configuration file must be named `<AppName>-config.xml`, where `<AppName>` is the string passed to the `Application` constructor.  It must reside in the working directory from which the application is launched (or in the path searched by the framework).

```
MyApp-config.xml   ←  loaded automatically when Application("MyApp") is constructed
```

---

## XML structure

### Root element

```xml
<configuration>
  <!-- variables and modules go here -->
</configuration>
```

The document element must be `<configuration>`.

---

### Scalar variables

```xml
<variable name="myValue" type="int32" value="42" />
```

| Attribute | Required | Description |
|-----------|----------|-------------|
| `name`    | yes      | Variable name (used as the path component) |
| `type`    | yes      | Data type (see table below) |
| `value`   | yes      | The scalar value as a string |

#### Supported types

| XML `type`  | C++ type           | Notes                          |
|-------------|--------------------|---------------------------------|
| `int8`      | `int8_t`           |                                |
| `uint8`     | `uint8_t`          |                                |
| `int16`     | `int16_t`          |                                |
| `uint16`    | `uint16_t`         |                                |
| `int32`     | `int32_t`          |                                |
| `uint32`    | `uint32_t`         |                                |
| `int64`     | `int64_t`          |                                |
| `uint64`    | `uint64_t`         |                                |
| `float`     | `float`            | Also usable as `double` in C++ |
| `double`    | `double`           |                                |
| `string`    | `std::string`      |                                |
| `boolean`   | `bool`             | Values: `true` / `false`       |

---

### Array variables

Omit the `value` attribute and instead provide `<value>` child elements, one per element:

```xml
<variable name="myArray" type="float">
  <value i="0" v="1.0" />
  <value i="1" v="2.5" />
  <value i="2" v="3.14" />
</variable>
```

| Attribute | Description            |
|-----------|------------------------|
| `i`       | Zero-based index       |
| `v`       | Element value (string) |

---

### Hierarchical modules (namespacing)

Use `<module name="...">` to group variables under a path prefix:

```xml
<module name="Sensors">
  <variable name="calibration" type="float" value="1.05" />
  <module name="Channel1">
    <variable name="offset" type="int32" value="10" />
  </module>
</module>
```

Variables are then accessed with the path `Sensors/calibration`, `Sensors/Channel1/offset`, etc.  Nesting depth is unlimited.

---

## Reading values in C++

Include `<ChimeraTK/ApplicationCore/ConfigReader.h>` (or use the `appConfig()` free function available inside `ApplicationModule::mainLoop()`).

```cpp
auto& cfg = appConfig();

// Scalar
int32_t gain = cfg.get<int32_t>("Sensors/Channel1/offset");

// Scalar with default (returned when the key is absent)
double timeout = cfg.get<double>("timeout", 5.0);

// Array
std::vector<float> cal = cfg.get<std::vector<float>>("Sensors/calibration");

// List sub-module names at a given path
std::vector<std::string> channels = cfg.getModules("Sensors");
// → {"Channel1"}
```

Path rules:
- Separator is `/`.
- A leading `/` is stripped automatically, so `"/foo"` and `"foo"` are equivalent.
- Names are case-sensitive.

---

## Reading values in Python

```python
cfg = appConfig()

s  = cfg.get(DataType.string,  "stringScalar")
f  = cfg.get(DataType.float64, "floatScalar")
arr = cfg.getArray(DataType.float64, "floatArray")

# With default
missing = cfg.get(DataType.int32, "doesNotExist", 42)

modules = cfg.getModules("SomeGroup")
```

---

## Reading values in Lua

```lua
local cfg = appConfig()

local s       = cfg:get(DataType.string,  "stringScalar")
local f       = cfg:get(DataType.float64, "floatScalar")
local arr     = cfg:getArray(DataType.float64, "floatArray")

-- With default
local missing = cfg:get(DataType.int32, "doesNotExist", 42)

local mods = cfg:getModules("SomeGroup")
```

---

## Special sections for scripted modules

### Lua modules

The `<LuaModules>` section declares which Lua scripts to load.  Each `<module>` child represents one script entry; the mandatory `path` variable points to the `.lua` file (resolved relative to the working directory).

```xml
<configuration>
  <module name="LuaModules">
    <module name="MyModule">
      <variable name="path" type="string" value="mymodule.lua" />
    </module>
    <module name="AnotherModule">
      <variable name="path" type="string" value="scripts/another.lua" />
    </module>
  </module>

  <!-- ordinary config variables accessible via appConfig() in all modules -->
  <variable name="setpoint" type="float" value="100.0" />
</configuration>
```

Multiple scripts can be declared; each gets its own Lua VM and runs in its own thread.

### Python modules

The `<PythonModules>` section follows the same pattern:

```xml
<configuration>
  <module name="PythonModules">
    <module name="MyPyModule">
      <variable name="path" type="string" value="mymodule.py" />
    </module>
  </module>
</configuration>
```

---

## Complete example

```xml
<configuration>

  <!-- Scripted modules -->
  <module name="LuaModules">
    <module name="Control">
      <variable name="path" type="string" value="control.lua" />
    </module>
  </module>

  <!-- Scalars at the top level -->
  <variable name="maxCurrent"  type="float"   value="5.0"    />
  <variable name="deviceName"  type="string"  value="DEV001" />
  <variable name="enableDebug" type="boolean" value="false"  />

  <!-- Nested namespace -->
  <module name="PID">
    <variable name="kP" type="double" value="1.2"  />
    <variable name="kI" type="double" value="0.05" />
    <variable name="kD" type="double" value="0.01" />
  </module>

  <!-- Array -->
  <variable name="lookupTable" type="float">
    <value i="0" v="0.0"  />
    <value i="1" v="0.25" />
    <value i="2" v="0.5"  />
    <value i="3" v="0.75" />
    <value i="4" v="1.0"  />
  </variable>

</configuration>
```

Corresponding C++ reads:

```cpp
auto& cfg = appConfig();
float maxI    = cfg.get<float>("maxCurrent");           // 5.0
double kP     = cfg.get<double>("PID/kP");              // 1.2
auto   lut    = cfg.get<std::vector<float>>("lookupTable"); // 5-element vector
```

---

## Control system visibility

All variables declared in the config file are automatically published as process variables in the control system hierarchy under `/Configurable/`.  This means operators can read (and, for writable variables, modify) them at run time through the standard control-system interface.
