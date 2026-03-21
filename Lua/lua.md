# Lua Scripting Support for ChimeraTK ApplicationCore

ApplicationCore can host ApplicationModules written in Lua 5.x or LuaJIT. The bindings mirror
the Python support and provide access to the same ApplicationCore concepts: scalars, arrays,
void accessors, ReadAnyGroup, DataConsistencyGroup, logging, and configuration reading.

---

## Build Requirements

The Lua feature is enabled automatically if either LuaJIT or a standard Lua 5.x installation is
found. Use the CMake options to control this:

```cmake
cmake -DENABLE_LUA_BINDINGS=ON  \  # default ON
      -DLUA_IMPL=AUTO            \  # AUTO | LUAJIT | LUAC
      ..
```

---

## Declaring Lua Modules in the Application Config

Add a `<LuaModules>` section to your application's XML configuration file (the same file used by
`ConfigReader`):

```xml
<configuration>
  <module name="LuaModules">
    <module name="MyController">
      <variable name="path" type="string" value="mycontroller.lua" />
    </module>
  </module>
</configuration>
```

Each `<module>` entry causes one Lua script file to be loaded. The `path` value is the file name
(absolute or relative to the working directory).

---

## Writing a Lua Module

A Lua script creates one or more `ApplicationModule` objects attached to the global `app` (a
`ModuleGroup`). Accessors are created **at script level** (before `app.initialise()`) and stored on
the module with `mod.myAccessor = ...`. The module's main loop receives `self` as its first argument
and retrieves accessors via `self.myAccessor`.

### Minimal example

```lua
-- mymodule.lua

local mod = ApplicationModule(app, "Control", "A simple controller",
  function(self)
    -- This is the mainLoop, running in a dedicated thread.
    self.output:setAndWrite(0.0)
    while true do
      local v = self.input:readAndGet()
      self.output:setAndWrite(v * 2.0)
    end
  end)

-- Accessors must be created at script level (before initialise).
mod.input  = ScalarPushInput(DataType.float32, mod, "/Measurement", "V",   "Raw measurement")
mod.output = ScalarOutput   (DataType.float32, mod, "/Setpoint",    "V",   "Computed setpoint")
```

> **Important**: The `mainLoop` function must access process variables **only** through `self`.
> It must not capture Lua variables from the outer script scope as upvalues, because the function
> is migrated to a dedicated per-module Lua VM that does not share state with the loading VM.
> Local variables defined *inside* the `mainLoop` function body are fine.

---

## API Reference

### DataType

A table of type constants passed to accessor factories:

```lua
DataType.int8   DataType.uint8   DataType.int16   DataType.uint16
DataType.int32  DataType.uint32  DataType.int64   DataType.uint64
DataType.float32 DataType.float64
DataType.string  DataType.Boolean  DataType.Void
```

### DataValidity

```lua
DataValidity.ok      DataValidity.faulty
```

### VersionNumber

```lua
local vn = VersionNumber()      -- new unique version
local vn = VersionNumber(nil)   -- null / uninitialised version
vn:isNullVersion()              -- returns true for the null version
```

### TransferElementID

Returned by `ReadAnyGroup:readAny()`. Compare with `==` or check with `isValid()`.

---

### Scalar accessors

| Factory                   | Direction / type       |
|---------------------------|------------------------|
| `ScalarPushInput`         | CS → module (push)     |
| `ScalarPushInputWB`       | CS → module (push, WB) |
| `ScalarPollInput`         | CS → module (poll)     |
| `ScalarOutput`            | module → CS            |
| `ScalarOutputPushRB`      | module → CS, with RB   |
| `ScalarOutputReverseRecovery` | special recovery   |

```lua
-- factory signature
ScalarPushInput(DataType.int32, owner, "/Path/name", "unit", "description")
```

**Methods** (on any scalar accessor):

```lua
acc:get()                   -- current value (Lua number / string / boolean)
acc:readAndGet()            -- blocking read, then return value
acc:set(value)              -- set in-memory value (no write to network)
acc:setAndWrite(value)      -- set + write
acc:writeIfDifferent(value) -- write only when value changed
acc:read()                  -- blocking read
acc:readNonBlocking()       -- returns true if new data arrived
acc:readLatest()            -- returns true if new data arrived
acc:write()
acc:getName()  acc:getUnit()  acc:getDescription()
acc:getValueType()  acc:getVersionNumber()  acc:getId()
acc:isReadOnly()  acc:isReadable()  acc:isWriteable()
acc:dataValidity()
```

---

### Array accessors

| Factory               | Same directions as scalars |
|-----------------------|----------------------------|
| `ArrayPushInput`      |                            |
| `ArrayPushInputWB`    |                            |
| `ArrayPollInput`      |                            |
| `ArrayOutput`         |                            |
| `ArrayOutputPushRB`   |                            |
| `ArrayOutputReverseRecovery` |                   |

```lua
ArrayPushInput(DataType.int32, owner, "/Path/name", "unit", 10, "description")
--                                                           ^--- nElements
```

**View semantics**: Array accessors are views into the C++ buffer — no data is copied. Index access
is 1-based (Lua convention).

```lua
arr:read()
arr:readAndGet()          -- read(), return self (for use in for-loops)
local n = #arr            -- number of elements (__len)
local v = arr[3]          -- get element 3 (__index)
arr[3] = 42               -- set element 3 (__newindex)

-- Iterate with pairs or ipairs:
for i, v in pairs(arr) do
  print(i, v)
end

-- Read then iterate:
for i, v in pairs(arr:readAndGet()) do
  print(i, v)
end
```

---

### Void accessors

```lua
VoidInput (owner, "/Path/name", "description")
VoidOutput(owner, "/Path/name", "description")
```

---

### ReadAnyGroup

```lua
local rag = ReadAnyGroup()
rag:add(self.input1)
rag:add(self.input2)
rag:finalise()

while true do
  local id = rag:readAny()
  if id == self.input1:getId() then
    -- self.input1 has new data
  elseif id == self.input2:getId() then
    -- self.input2 has new data
  end
end
```

---

### DataConsistencyGroup

```lua
local dcg = DataConsistencyGroup(MatchingMode.exact)
dcg:add(self.ts)
dcg:add(self.data)

while true do
  local id = rag:readAny()
  if dcg:update(id) then
    -- both ts and data are consistent
  end
end
```

---

### Logging

```lua
logger(Severity.info, "MyModule"):log("Startup complete")
logger(Severity.warning, "MyModule"):log("Unexpected value: " .. tostring(v))
```

Severity levels: `trace`, `debug`, `info`, `warning`, `error`.

---

### Application config reader

```lua
local cfg = appConfig()
local threshold = cfg:get(DataType.float64, "MyModule/threshold", 1.0)
local coeffs    = cfg:getArray(DataType.float64, "MyModule/coefficients")
```

---

### VariableGroup and ModuleGroup

```lua
-- VariableGroup inside a module
local vg = VariableGroup(mod, "Sensors", "Sensor inputs")
local temp = ScalarPushInput(DataType.float32, vg, "Temperature", "°C", "")

-- ModuleGroup for nesting
local grp = ModuleGroup(app, "Controllers", "")
local sub = ApplicationModule(grp, "PID", "PID controller", function(self) ... end)
```

---

## LSP Setup (lua-language-server)

The stub file `applicationcore.lua` is installed to:

```
<prefix>/share/ChimeraTK/ApplicationCore/lua-stubs/
```

Add it to your project's `.luarc.json`:

```json
{
  "workspace.library": [
    "/usr/share/ChimeraTK/ApplicationCore/lua-stubs"
  ]
}
```

This enables completion, hover documentation and type checking for all ApplicationCore globals in
editors that use [lua-language-server](https://github.com/LuaLS/lua-language-server) (VS Code with
the `sumneko.lua` extension, Neovim, etc.).

---

## Concurrency Model

Each Lua module runs in its own `sol::state` (Lua VM) and its own C++ thread. Modules execute
concurrently without any shared Lua state. The underlying ApplicationCore C++ operations (read,
write, etc.) are already thread-safe.
