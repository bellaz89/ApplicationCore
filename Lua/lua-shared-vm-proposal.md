# Proposal: Loading + Migration Architecture for Lua Modules

**Status**: Draft
**Branch**: `lua`

---

## Motivation

The current implementation has two problems:

1. **Upvalue limitation** — `mainLoop` is serialised as bytecode and replayed
   in a per-module VM.  `lua_dump` does not carry upvalue values, so file-scope
   locals captured by the function become nil in the module VM.  Accessors
   cannot be captured as locals; every value the loop needs must be re-fetched
   at runtime via `_namedAccessors` / `__index`.

2. **API friction** — accessor factories require an explicit `owner` argument,
   `appConfig()` returns a proxy object for three read-only calls, and
   `logger()` returns a one-method proxy object.  These are C++ OOP idioms
   that do not belong in a Lua API.

---

## Design

### Loading phase → per-module VM migration

```
Loading thread (one sol::state)
  │
  ├── runs the script once
  ├── creates all C++ objects (ModuleGroup, ApplicationModule, accessors)
  └── populates each module's property table
          │
          ├── ctrl.setpoint  →  LuaScalarAccessor& (C++ owned)
          ├── ctrl.gain      →  1.5               (Lua number)
          ├── ctrl.limits    →  {-10, 10}          (Lua table)
          └── ctrl.mainLoop  →  function           (bytecode)
          │
          ▼
  Migration: for each module, create a fresh sol::state and copy its table

Module thread 1  owns  sol::state for ctrl  →  calls ctrl:mainLoop()
Module thread 2  owns  sol::state for mon   →  calls mon:mainLoop()
Module thread 3  owns  sol::state for hb    →  calls hb:mainLoop()
```

- The loading thread runs the script to completion, wiring the full C++
  hierarchy (`ModuleGroup` → `ApplicationModule` → accessors).  The Lua VM
  used during loading is then freed.

- Each module's property table is migrated into a fresh `sol::state` owned by
  that module's OS thread.  The `lua_State` is never shared between threads —
  LuaJIT is fully safe.

- `mainLoop` is a regular Lua function.  It is bytecode-transferred to the
  module VM at migration time, with all upvalues patched in via the debug API.

### What migrates and how

Two complementary mechanisms handle migration.

**Module properties** (values assigned to a module via `mod.x = ...`) are
intercepted by `__newindex` at load time and stored as C++ factory lambdas.
At migration the factory is called in the target VM to produce a properly-typed
Lua object:

| Property type | Storage | Migration |
|---|---|---|
| Number / string / boolean | Captured as C++ value in lambda | Lambda called in target VM |
| Table of the above | Recursive deep copy into C++ structure | Reconstructed as Lua table in target VM |
| C++ userdata (accessors, groups) | Raw C++ pointer captured in lambda | Lambda wraps pointer for target VM |

**Function upvalues** (locals captured by `mainLoop` or any helper) are not
carried by `lua_dump`.  After `lua_load` reconstructs the closure in the target
VM, the migration reads each upvalue from the source with `lua_getupvalue`
and patches it into the target closure with `lua_setupvalue` (the C API
equivalents of `debug.getupvalue`/`debug.setupvalue`):

| Upvalue type | Migration |
|---|---|
| Number / string / boolean | Extracted as C++ value, set via `debug.setupvalue` |
| Table of the above | Recursive deep copy into target VM, set via `debug.setupvalue` |
| C++ userdata (accessors, groups) | New Lua wrapper around the same C++ pointer, set via `debug.setupvalue` |
| Lua function | `lua_dump` → `lua_load` in target VM, then recursively migrate its upvalues |
| Coroutine | Not migratable — load-time error if encountered as an upvalue |

**`_ENV` handling**: Lua 5.2+ compiles global variable access through an explicit
`_ENV` upvalue (always index 1 of any function touching globals).  `lua_load`
sets it to the target VM's `_G` automatically — the migration must skip it.
LuaJIT (Lua 5.1) uses `GETGLOBAL`/`SETGLOBAL` opcodes instead; there is no
`_ENV` upvalue.  A single runtime name check handles both without preprocessor
conditionals:

```cpp
const char* name = lua_getupvalue(L, funcIdx, i);
if(name && std::string_view(name) == "_ENV") continue;  // Lua 5.2+ only; no-op on LuaJIT
// migrate upvalue i
```

### Upvalue migration — no `self` discipline required

Because upvalues are fully migrated, file-scope locals work naturally inside
`mainLoop`.  The `self.xxx` pattern is still valid but no longer mandatory:

```lua
local gain   = config.get("gain", 1.0)        -- number upvalue
local limits = config.getArray("limits", {-10.0, 10.0})  -- table upvalue
local input  = ctrl:ScalarPushInput(DataType.float32, "input", "V", "")  -- userdata upvalue
local output = ctrl:ScalarOutput   (DataType.float32, "output", "V", "")

function ctrl:mainLoop()
    while true do
        local v = input:readAndGet() * gain
        v = math.max(limits[1], math.min(limits[2], v))
        output:writeIfDifferent(v)
    end
end
```

All of `gain`, `limits`, `input`, and `output` are upvalues of `ctrl:mainLoop`.
The migration copies them into the module VM before the thread starts.

**Remaining caveat — shared upvalue cells**: two closures that share the same
upvalue *cell* (not just the same initial value) become independent copies after
migration.  This only matters if both closures mutate the shared variable and
expect the mutation to be visible to each other — an unusual pattern for
ChimeraTK modules, where inter-module communication goes through process
variables.

```lua
local n = 0
function ctrl:mainLoop()  n = n + 1 end   -- upvalue cell shared with getCount
function ctrl:getCount()  return n end     -- after migration: independent copy of n
```

For immutable upvalues (constants, config values, accessors) the distinction
does not exist.

### Implicit owner

`ApplicationModule`, `ModuleGroup`, and `VariableGroup` attach to the
application root by default.  Passing `app` explicitly is dropped.  Nested
ownership is expressed by passing the parent as the first argument:

```lua
local grp  = ModuleGroup("Subsystem", "")          -- attaches to app root
local ctrl = ApplicationModule(grp, "Worker", "")  -- attaches to grp
local vg   = ctrl:VariableGroup("Sensors", "")     -- method call on module
```

### Accessor factories as method calls

Accessors are declared as method calls on their owner.  The owner is implicit:

```lua
ctrl.input = ctrl:ScalarPushInput(DataType.float32, "input", "V", "desc")
vg.temp    = vg:ScalarPushInput  (DataType.float32, "temp",  "°C", "desc")
```

### Config — free functions, type from XML

`config.get` and `config.getArray` read the type from the XML `type`
attribute and return the matching Lua primitive.  No `DataType` argument:

```lua
ctrl.gain   = config.get("path/gain",   1.0)    -- number
ctrl.name   = config.get("path/name",   "PID")  -- string
ctrl.active = config.get("path/active", true)   -- boolean
ctrl.coefs  = config.getArray("path/coefs", {1.0, 0.5})  -- table of numbers
```

The second argument is an optional default; omitting it throws if the path is
missing in the XML.

### Conditional modules

Because `ApplicationModule(...)` is a factory call at file level, a module can
be conditionally excluded by simply not creating it — no `disable()` call
needed:

```lua
if config.get("Plant/Controller/enabled", true) then
    local ctrl = ApplicationModule("Controller", "desc")
    ctrl.input  = ctrl:ScalarPushInput(DataType.float32, "input",  "V", "desc")
    ctrl.output = ctrl:ScalarOutput   (DataType.float32, "output", "V", "desc")

    function ctrl:mainLoop()
        while true do
            self.input:read()
            self.output:setAndWrite(self.input:get())
        end
    end
end
```

This is cleaner than Python's pattern, where the constructor is mandatory and
`self.disable()` must be called from inside it.  `disable()` is still bound as
a method for cases where partial setup precedes the decision, but the `if`-guard
is the idiomatic Lua form.

### UserInputValidator

`UserInputValidator` is a utility (not a module) that validates operator inputs
against conditions expressed as Lua functions.  When a value fails validation
it is reverted to the last known good value (or a configured fallback) and an
error function is called.  It integrates with `ReadAnyGroup` — each result of
`readAny()` is passed to `validate()`.

The validator is created inside `mainLoop`, so it is never subject to migration:

```lua
function ctrl:mainLoop()
    local validator = UserInputValidator()

    validator:add(
        "gain must be positive",
        function() return self.gain:get() > 0 end,
        self.gain)

    validator:add(
        "gain must not exceed 100",
        function() return self.gain:get() <= 100 end,
        self.gain)

    validator:setFallback(self.gain, 1.0)

    validator:setErrorFunction(function(msg)
        log(Severity.warning, self.logCtx, msg)
        self.status:setAndWrite("invalid: " .. msg)
    end)

    local rag = ReadAnyGroup()
    rag:add(self.setpoint)
    rag:add(self.gain)
    rag:finalise()

    validator:validateAll()   -- validate initial values before first read

    while true do
        local id = rag:readAny()
        validator:validate(id)
        self.output:setAndWrite(self.setpoint:get() * self.gain:get())
    end
end
```

sol2 converts Lua functions to `std::function<bool()>` and
`std::function<void(string)>` natively, the same mechanism pybind11 uses via
`py::functional.h`.  This is a missing binding, not a fundamental limitation.

### Logging — free function

```lua
log(Severity.info,    "context", "message")
log(Severity.warning, "context", "message")
```

### Accessor arithmetic operators and `__tostring`

Scalar accessors expose numeric metamethods so that `mainLoop` expressions
read naturally without explicit `:get()` calls:

```lua
-- without operators (current)
self.output:setAndWrite(self.input:get() * self.gain:get() + self.offset:get())

-- with operators
self.output:setAndWrite(self.input * self.gain + self.offset)
```

Supported metamethods via sol2 `sol::meta_function`:

| Lua metamethod | Operation |
|---|---|
| `__add`, `__sub`, `__mul`, `__div` | binary arithmetic with another accessor or a plain number |
| `__unm` | unary minus |
| `__eq`, `__lt`, `__le` | comparison (returns boolean) |
| `__tostring` | debug representation for `tostring()` and `print()` |

`__tostring` returns a human-readable string useful in log messages:

```lua
log(Severity.debug, self.logCtx, "input = " .. tostring(self.input))
-- prints: input = <ScalarAccessor name=/Plant/Controller/input value=1.5 type=float32 validity=ok>
```

In Lua 5.3+ the `__add` metamethod is tried on either operand, so both
`self.input + 1.0` and `1.0 + self.input` work correctly.

This is directly equivalent to Python's numeric emulation
(`specialFunctionsToEmulateNumeric`) and improves on it — Python requires
`self.input + 1.0` to go through `__add__` on the left operand only, while
Lua's symmetric metamethod dispatch handles both orderings uniformly.

---

## Reference Example

```lua
-- plant_control.lua
--
-- Demonstrates the current proposal API:
--   · multiple modules in one file
--   · module group and variable group
--   · method-call accessor factories  (mod:ScalarPushInput(...))
--   · config.get / config.getArray  (type inferred from XML)
--   · log()
--   · ReadAnyGroup, DataConsistencyGroup
--   · scalar, array, void accessors
--   · self-based mainLoop  (self.xxx and local upvalues both migration-safe)

-- ── Module group ──────────────────────────────────────────────────────────────
local grp = ModuleGroup("Plant", "Plant control subsystem")

-- ── Module 1: Controller ──────────────────────────────────────────────────────
local ctrl = ApplicationModule(grp, "Controller", "PID control loop")

ctrl.setpoint = ctrl:ScalarPushInput  (DataType.float32, "setpoint", "V", "Operator setpoint")
ctrl.readback = ctrl:ScalarPushInputWB(DataType.float32, "readback", "V", "Plant readback")
ctrl.output   = ctrl:ScalarOutput     (DataType.float32, "output",   "V", "Control output")
ctrl.gain     = ctrl:ScalarPollInput  (DataType.float32, "gain",     "",  "Loop gain")

ctrl.outMin   = config.get("Plant/Controller/outputMin", -10.0)
ctrl.outMax   = config.get("Plant/Controller/outputMax",  10.0)
ctrl.logCtx   = config.get("Plant/Controller/name",      "PID")
ctrl.limits   = config.getArray("Plant/Controller/softLimits", {-10.0, 10.0})

function ctrl:mainLoop()
    log(Severity.info, self.logCtx, "starting")

    local rag = ReadAnyGroup()
    rag:add(self.setpoint)
    rag:add(self.readback)
    rag:finalise()

    log(Severity.info, self.logCtx, "running")

    while true do
        rag:readAny()
        self.gain:readLatest()

        local out = self.setpoint:get() * self.gain:get() - self.readback:get()
        out = math.max(self.limits[1], math.min(self.limits[2], out))
        self.output:writeIfDifferent(out)
    end
end

-- ── Module 2: Monitor ─────────────────────────────────────────────────────────
local mon = ApplicationModule(grp, "Monitor", "Output monitor")

local sig         = mon:VariableGroup("Signals", "Monitored signals")
mon.monOutput     = sig:ScalarPushInput(DataType.float32, "output",    "V", "Output to monitor")
mon.monReadback   = sig:ScalarPushInput(DataType.float32, "readback",  "V", "Readback to monitor")

mon.alarm         = mon:ScalarOutput   (DataType.bool,    "alarm",     "",  "Alarm flag")
mon.history       = mon:ArrayOutput    (DataType.float32, "history",   "V", 64, "Ring buffer")
mon.alarmThr      = mon:ScalarPollInput(DataType.float32, "threshold", "V", "Alarm threshold")

mon.logCtx        = config.get("Plant/Monitor/logContext", "Monitor")
mon.enabled       = config.get("Plant/Monitor/enabled",    true)

function mon:mainLoop()
    if not self.enabled then
        log(Severity.warning, self.logCtx, "monitor disabled by config")
        return
    end

    local dcg = DataConsistencyGroup(MatchingMode.exact)
    dcg:add(self.monOutput)
    dcg:add(self.monReadback)

    local rag = ReadAnyGroup()
    rag:add(self.monOutput)
    rag:add(self.monReadback)
    rag:finalise()

    local head = 1   -- local inside mainLoop: fine, not subject to migration

    while true do
        local id = rag:readAny()
        self.alarmThr:readLatest()

        if dcg:update(id) then
            local v = self.monOutput:get()
            self.alarm:setAndWrite(math.abs(v) > self.alarmThr:get())

            self.history[head] = v
            head = (head % #self.history) + 1
            self.history:write()
        end
    end
end

-- ── Module 3: Heartbeat — void accessors, attaches to app root ───────────────
local hb   = ApplicationModule("Heartbeat", "Periodic heartbeat")

hb.trigger = hb:VoidInput ("trigger",   "Periodic trigger from timer")
hb.beat    = hb:VoidOutput("heartbeat", "Heartbeat pulse to watchdog")
hb.count   = hb:ScalarOutput(DataType.uint64, "count",  "", "Tick counter")
hb.status  = hb:ScalarOutput(DataType.string, "status", "", "Last tick info")

function hb:mainLoop()
    local n = 0
    while true do
        self.trigger:read()
        n = n + 1
        self.count:setAndWrite(n)
        self.status:setAndWrite("tick " .. tostring(n))
        self.beat:write()
    end
end
```

---

## Python vs Lua — feature comparison

| Feature | Python | Lua (proposed) |
|---|---|---|
| Multiple modules per file | ✓ multiple classes in one `.py` | ✓ multiple `ApplicationModule(...)` calls |
| Module groups / nesting | Pass parent to constructor | `ModuleGroup(parent, ...)` — same pattern |
| Accessor declaration | `self.x = ScalarPushInput(type, self, ...)` in `__init__` | `mod.x = mod:ScalarPushInput(type, ...)` at file level |
| Config access | `self.config.get(type, path, default)` | `config.get(path, default)` — type from XML |
| Logging | `self.logger.info(msg)` | `log(Severity.info, ctx, msg)` |
| mainLoop access style | `self.x` (instance attribute) | `self.x` or local upvalue — both work |
| Arithmetic on accessors | ✓ `self.input + 1.0` via `__add__` | ✓ `self.input + 1.0` via `__add` — symmetric dispatch | Lua has edge over Python (both operand orderings work) |
| Debug representation | ✓ `repr(self.input)` via `__repr__` | ✓ `tostring(self.input)` via `__tostring` | Symmetric |
| UserInputValidator | ✓ | ✓ missing binding, not a limitation — Lua closures work identically |
| Conditional module (disable) | `self.disable()` called inside `__init__` after mandatory constructor | `if config.get(...) then ... end` — module not created at all | Lua cleaner |
| Per-module initialisation | `__init__` runs per instance | file-level code runs once; properties are per-module |
| Shared file-level Lua state | ✓ module globals shared across instances | ✗ each module VM is isolated after migration |
| LuaJIT safe | N/A | ✓ each VM owned by exactly one thread |
| PeriodicTrigger | ✗ not bound | ✓ planned — load-time instantiation |
| Status accessors + enum | ✗ not bound | ✓ planned — same pattern as scalar accessors |
| StatusAggregator | ✗ not bound | ✓ planned — load-time instantiation |
| StatusMonitor variants | ✗ not bound | ◑ planned — all numeric types via `callForTypeNoVoid` |
| FanIn | ✗ not bound | — deferred |

---

## Runtime Compatibility

The design is feasible on both target runtimes.

| Mechanism | Lua 5.2 / 5.3 / 5.4 | LuaJIT (Lua 5.1) |
|---|---|---|
| `lua_dump` on inner functions | ✓ | ✓ |
| `luaL_loadbuffer` with VM bytecode | ✓ | ✓ (LuaJIT bytecode format, same process) |
| `lua_getupvalue` / `lua_setupvalue` | ✓ | ✓ |
| `_ENV` as explicit upvalue | ✓ — set by `lua_load`, skipped by name check | ✗ — no `_ENV` upvalue; name check is a no-op |
| Native 64-bit integers | ✓ Lua 5.3+ | ✗ doubles only — see limitation below |
| One `lua_State` per OS thread | ✓ | ✓ — required by LuaJIT; guaranteed by design |
| sol2 support | ✓ | ✓ via `SOL_LUAJIT` cmake define |

**JIT compilation and `lua_dump`**: LuaJIT only JIT-compiles functions after
they are executed.  `mainLoop` is defined but never called during the loading
phase, so it is never JIT-compiled before `lua_dump` runs.  `lua_dump` operates
on the Lua bytecode prototype regardless of JIT state.

---

## Limitations compared to Python

### 1. No shared Lua state between co-file modules

After migration each module has its own isolated VM.  Two modules declared in
the same file cannot share a Lua variable (calibration table, shared counter).
Communication between modules must go through ChimeraTK process variables.

Python class instances from the same module can share module-level globals.
This capability is absent in the migration approach.

**Mitigation**: values known at load time (constants, config-derived tables)
can be assigned as properties on each module independently — they are copied
at migration, not shared, but for read-only data this is equivalent.

### 2. Shared upvalue cells are broken after migration

If two closures share the same upvalue *cell* and both mutate it, migration
makes them independent — each gets its own copy of the initial value.  This
only matters for mutable shared state between two closures in the same module,
which is an unusual pattern.  Immutable upvalues (constants, config values,
accessors) are unaffected.

### 3. No per-instance initialiser

Python's `__init__` runs once per instance with a scoped `self`.  In Lua,
file-level code runs once for the entire file — there is no hook that runs
per-module with a distinct `self`.  Per-module state must be encoded as
distinct named properties:

```lua
ctrl.integral = 0.0   -- explicit per-module state
mon.head      = 1
```

### 4. int64 / uint64 precision on LuaJIT

LuaJIT represents all Lua numbers as `double` (53-bit mantissa).  Values of
type `DataType.int64` or `DataType.uint64` exceeding 2^53 lose precision when
passed through the Lua layer.  Lua 5.3+ has native 64-bit integers and is not
affected.

This is a pre-existing limitation of the LuaJIT number model, not introduced by
the migration design.  Applications that require exact 64-bit arithmetic in
LuaJIT should use `DataType.int32` / `DataType.uint32` or treat the accessor
value as an opaque token (e.g. tick counters that are only compared, never
arithmetically manipulated).

### 5. No type system or decorators

Python modules benefit from type annotations, dataclasses, and IDE tooling.
Lua has none of these; the only IDE support is the hand-written stub file.

---

## Additional C++ modules worth binding

The following modules exist in `Modules/include/` and are not exposed in the
Python bindings.  Lua is a natural home for them because they are instantiated
once at load time and never subject to migration.

### PeriodicTrigger  *(high priority — very easy to implement)*

`PeriodicTrigger` is a complete `ApplicationModule` that emits a `tick` counter
at a configurable interval (milliseconds).  It replaces the common pattern of
wiring a `VoidInput` trigger from an external C++ timer.

```lua
-- load time — attaches to grp, default period 100 ms
local pt = PeriodicTrigger(grp, "Timer", "100 ms tick", 100)

-- Another module reads the tick output by path through the ChimeraTK network.
hb.trigger = hb:ScalarPushInput(DataType.uint64, "Timer/tick", "", "Periodic trigger")
```

The `period` input (`ScalarPollInput<uint32_t>`) is exposed automatically by the
C++ module and is writable from the control system at runtime.

**Implementation**: factory function in `LuaBindings.cc` calling
`make_child<PeriodicTrigger>()` on the owner — the existing ownership mechanism
handles the rest.  No new files, no Lua VM involvement after construction.
~20 lines.

---

### Status accessors + `Status` enum  *(high priority — moderate effort)*

`StatusOutput`, `StatusPushInput`, `StatusPollInput` are typed variants of the
standard scalar accessors, restricted to the four-value `Status` enum:

```lua
Status.OFF      -- 0
Status.OK       -- 1
Status.WARNING  -- 2
Status.FAULT    -- 3
```

Without these, a Lua module cannot report health into `StatusAggregator` or
read a status input from another module.

```lua
mod.health = mod:StatusOutput   ("health", "Module health")
mod.devSt  = mod:StatusPushInput("Device/status", "Device status")

function mod:mainLoop()
    while true do
        self.devSt:read()
        if self.devSt:get() == Status.FAULT then
            self.health:setAndWrite(Status.FAULT)
        else
            self.health:setAndWrite(Status.OK)
        end
    end
end
```

**Implementation**: a new `LuaStatusAccessor` class (one header + one source
file) holding `std::variant<StatusOutput, StatusPushInput, StatusPollInput>`.
Because the status accessors are non-template (always `int32_t` internally),
`LuaStatusAccessor` is significantly simpler than `LuaScalarAccessor` — the
variant has three fixed types instead of ~20.  `get()` returns a Lua integer
comparable to `Status.OK` etc.  The `Status` enum table is 4 lines.
~200 lines total.

---

### StatusAggregator  *(high priority — easy to implement)*

`StatusAggregator` collects `StatusOutput` values from a subtree of the module
hierarchy and exposes a combined worst-case (or configurable) status.  It is
instantiated once at load time with a `PriorityMode`.

> **Note**: the constructor searches the hierarchy for existing `StatusOutput`s,
> so the `StatusAggregator` must be created *after* all the monitors it should
> aggregate.

```lua
-- PriorityMode controls aggregation order:
--   fwok        — FAULT > WARNING > OK > OFF
--   fwko        — FAULT > WARNING > OK, treat OFF as OK
--   fw_warn_mixed — FAULT > WARNING, mixed OK/OFF → WARNING
--   ofwk        — OFF > FAULT > WARNING > OK

local agg = StatusAggregator(grp, "Health", "Subsystem health", PriorityMode.fwok)
```

No `mainLoop` in Lua — the aggregation runs in C++.

**Implementation**: factory function in `LuaBindings.cc` + `PriorityMode` enum
table.  Same pattern as `PeriodicTrigger`.  ~30 lines, no new files.

---

### StatusMonitor variants  *(moderate priority — moderate effort)*

`MaxMonitor<T>`, `MinMonitor<T>`, `RangeMonitor<T>`, and `ExactMonitor<T>` are
`ApplicationModule` subclasses that watch a scalar process variable and emit a
`StatusOutput` when it crosses warning/fault thresholds.

```lua
-- All paths are relative or absolute in the ChimeraTK hierarchy.
-- The parameterPath groups threshold variables (upperWarningThreshold, upperFaultThreshold, disable).
MaxMonitor   (DataType.float32, grp, "/Plant/temperature", "/Plant/TempStatus", "/Plant/TempParams", "Temp guard")
MinMonitor   (DataType.float32, grp, "/Plant/pressure",    "/Plant/PresStatus", "/Plant/PresParams",  "Press guard")
RangeMonitor (DataType.float32, grp, "/Plant/flow",        "/Plant/FlowStatus", "/Plant/FlowParams",  "Flow guard")
ExactMonitor (DataType.int32,   grp, "/Plant/mode",        "/Plant/ModeStatus", "/Plant/ModeParams",  "Mode guard")
```

Because these are fire-and-forget from Lua (no object is returned, no methods
to call), no new Lua usertype is needed.  The template type is selected via a
`DataType` argument dispatched with `callForTypeNoVoid`, the same mechanism used
by the scalar accessor factories:

```cpp
// sketch of MaxMonitor factory
lua.set_function("MaxMonitor",
    [](ChimeraTK::DataType type, LuaModuleGroup& owner,
       const std::string& input, const std::string& output,
       const std::string& params, const std::string& desc) {
        callForTypeNoVoid(type, [&](auto t) {
            using T = decltype(t);
            dynamic_cast<LuaOwningObject&>(owner).make_child<MaxMonitor<T>>(
                &owner, input, output, params, desc);
        });
    });
```

**Implementation**: four factory functions, ~20 lines each.  Bind for all
numeric types via `callForTypeNoVoid` — no artificial restriction to float32
and int32.  ~100 lines total in `LuaBindings.cc`, no new files.

---

## Summary

The loading + migration approach solves both the upvalue limitation and API
friction with low implementation complexity.  Each module VM is owned by exactly
one thread — LuaJIT is safe without any scheduler or mutex.

Upvalue migration (`debug.getupvalue` + `debug.setupvalue`) means that
file-scope locals — including config values, constants, and C++ accessors —
are fully available inside `mainLoop` without any `self.xxx` discipline.  The
`self`-based style remains valid and may be preferred for clarity, but it is no
longer mandatory.

The only remaining trade-off is that file-scope shared Lua state between
co-file modules is not supported — each module VM is isolated after migration.
For ChimeraTK applications this is acceptable: modules communicate through
process variables, not shared memory, and read-only config data can be copied
independently to each module at migration time.
