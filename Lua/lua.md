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

A Lua script **must** create exactly one `ApplicationModule` object and return it. Multiple modules cannot be instantiated from a single script file.

The script executes directly in the module's thread, so there is no bytecode capture or state migration overhead. Scripts work exactly as before:
1. Call `ApplicationModule("ModuleName", "Description")` to get the module instance
2. Create accessors using `mod:ScalarPushInput()` etc. (at script level)
3. Define the main loop as `function mod:mainLoop()` 
4. Return the module with `return mod`

Accessors are created **at script level** (before `app.initialise()`) and retrieved in `mainLoop` via `self.myAccessor`.

### Minimal example

```lua
-- mymodule.lua

local mod = ApplicationModule("Control", "A simple controller")
local cfg = appConfig()

-- Accessors and config values assigned as self properties at script level.
mod.input  = mod:ScalarPushInput(DataType.float32, "input",  "V", "Raw measurement")
mod.output = mod:ScalarOutput   (DataType.float32, "output", "V", "Computed setpoint")
mod.scale  = cfg:get("Control/scale", 2.0)

function mod:mainLoop()
    self.output:setAndWrite(0.0)
    while true do
        local v = self.input:readAndGet()
        self.output:setAndWrite(v * self.scale)
    end
end

-- IMPORTANT: Return the module so it can be registered with the application
return mod
```

> **Important**: Each Lua script **must** end with `return mod` to return the single ApplicationModule it creates. If a script returns nothing (nil), or returns a non-ApplicationModule value, an exception will be thrown and the script will fail to load.

> **Tip**: File-scope locals (including accessor objects) are automatically available inside `mainLoop` as upvalues. Since the script executes directly in the module's thread (not in a shared loader), all locals naturally persist without any special migration logic.

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
-- factory signature (method on module or VariableGroup)
mod:ScalarPushInput(DataType.int32, "name", "unit", "description")
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
mod:ArrayPushInput(DataType.int32, "name", "unit", 10, "description")
--                                                  ^--- nElements
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
mod:VoidInput ("name", "description")
mod:VoidOutput("name", "description")
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
log(Severity.info,    "MyModule", "Startup complete")
log(Severity.warning, "MyModule", "Unexpected value: " .. tostring(v))
```

Severity levels: `trace`, `debug`, `info`, `warning`, `error`.

---

### Application config

```lua
local cfg       = appConfig()
local threshold = cfg:get     ("MyModule/threshold", 1.0)
local coeffs    = cfg:getArray("MyModule/coefficients")
local modules   = cfg:getModules("MyModule/channels")
```

The type is read from the XML and converted automatically to the matching Lua type (number, string,
or boolean). `appConfig()` returns an object whose `:get` and `:getArray` methods accept an optional
default as the second argument; omitting it throws if the path is missing. `cfg:getModules` returns
a list of child module names under the given path.

---

### StatusAccessor

Use `StatusOutput`, `StatusPushInput`, or `StatusPollInput` to create status accessors. The status
value is one of the integer constants in the `Status` table.

```lua
local s = mod:StatusOutput("status", "Aggregated status")

-- In mainLoop:
s:setAndWrite(Status.OK)
s:setAndWrite(Status.WARNING)
s:setAndWrite(Status.FAULT)
s:setAndWrite(Status.OFF)

local v = s:get()         -- returns an integer
s:read()                   -- blocking read (for inputs)
s:readAndGet()             -- read + return integer
```

Status constants: `Status.OFF`, `Status.OK`, `Status.WARNING`, `Status.FAULT`.

---

### VariableGroup

```lua
-- VariableGroup inside a module
local vg   = VariableGroup(mod, "Sensors", "Sensor inputs")
local temp = vg:ScalarPushInput(DataType.float32, "Temperature", "°C", "")
local st   = vg:StatusOutput("health", "Sensor health")
```

---

### Upvalues and File-Scope Locals

File-scope locals are automatically available inside `mainLoop` as upvalues. This includes plain values (numbers, strings, booleans) and accessor objects. Since the script now executes directly in the module's thread (not in a shared loader), all locals naturally persist in the same Lua state:

```lua
local cfg   = appConfig()
local scale = cfg:get("MyModule/scale", 1.0)    -- number upvalue
local label = cfg:get("MyModule/label", "v=")   -- string upvalue

local mod    = ApplicationModule("MyModule", "Demo")
local input  = mod:ScalarPushInput(DataType.float32, "input",  "V", "")
local output = mod:ScalarOutput   (DataType.float32, "output", "V", "")

function mod.mainLoop(self)
    -- All of the above are available here as upvalues:
    log(Severity.info, label, "starting (scale=" .. tostring(scale) .. ")")
    output:setAndWrite(0.0)
    while true do
        local v = input:readAndGet()
        output:setAndWrite(v * scale)
    end
end
```

> **Note**: Accessor objects captured as upvalues (`input`, `output`) are the same C++ objects — their lifetimes are managed by the module group, not by the Lua VM.

---

### Scalar accessor arithmetic

Scalar accessors support arithmetic operators directly, so you can write expressions without calling
`:get()` explicitly:

```lua
self.sum:setAndWrite(self.a + self.b)    -- addition
self.diff:setAndWrite(self.a - self.b)   -- subtraction
self.prod:setAndWrite(self.a * self.b)   -- multiplication
self.quot:setAndWrite(self.a / self.b)   -- division
self.neg:setAndWrite(-self.a)            -- unary minus
local less = self.a < self.b             -- comparison (returns boolean)
```

Mixed accessor–number expressions also work: `self.output:setAndWrite(self.input + 10.0)`.

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

## Key Constraints and Requirements

### Single ApplicationModule Per Script

Each Lua script **must** create and return exactly **one** `ApplicationModule`:

- If a script creates multiple modules, an exception is thrown
- If a script returns `nil` or a non-ApplicationModule value, an exception is thrown
- The script's return value must be the same C++ instance created by the framework

This constraint simplifies the architecture: each script = each module = each thread.

### No Shared Loader State

Unlike earlier versions, there is **no shared Lua state** for loading scripts. Each module's script
runs directly in its own thread. This means:

- No bytecode capture or reconstruction overhead
- No need to serialize/deserialize Lua values between VMs
- File-scope locals naturally persist as upvalues
- Direct execution in the module's thread where `mainLoop` will run

---

Each Lua module runs in its own `sol::state` (Lua VM) and its own C++ thread. Modules execute
concurrently without any shared Lua state. The underlying ApplicationCore C++ operations (read,
write, etc.) are already thread-safe.

### Script Execution Flow

1. **Creation**: `LuaModuleManager` creates a `LuaApplicationModule` C++ object with the script path
2. **Thread Start**: The module thread starts and calls `run()`
3. **State Setup**: A new Lua state is created in that thread
4. **Script Execution**: The script file is loaded and executed in that same thread's Lua state
5. **Module Binding**: During script execution, `ApplicationModule("Name", "Desc")` retrieves the already-created C++ instance
6. **Setup Phase**: Script creates accessors at module level (before `app.initialise()`)
7. **Main Loop**: After setup, `mainLoop` is called repeatedly in the same thread/state

This direct in-thread execution eliminates the complexity of migrating Lua state between threads, since everything happens in the same VM and thread from start to finish.
