---@meta
-- SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- EmmyLua / lua-language-server annotation stubs for the ChimeraTK ApplicationCore Lua bindings.
-- These stubs enable code completion, hover documentation and type checking in LuaLS-aware editors.

-- ============================================================
--  DataType
-- ============================================================

---@class DataType
---@field int8    DataType
---@field uint8   DataType
---@field int16   DataType
---@field uint16  DataType
---@field int32   DataType
---@field uint32  DataType
---@field int64   DataType
---@field uint64  DataType
---@field float32 DataType
---@field float64 DataType
---@field string  DataType
---@field Boolean DataType
---@field Void    DataType
DataType = {}

-- ============================================================
--  DataValidity
-- ============================================================

---@class DataValidity
---@field ok     DataValidity
---@field faulty DataValidity
DataValidity = {}

-- ============================================================
--  VersionNumber
-- ============================================================

---@class VersionNumber
---@return VersionNumber
local VersionNumber = {}

---Returns true if this is the null (uninitialised) version number.
---@return boolean
function VersionNumber:isNullVersion() end

-- ============================================================
--  TransferElementID
-- ============================================================

---@class TransferElementID
local TransferElementID = {}

---Returns true when the ID refers to a valid transfer element.
---@return boolean
function TransferElementID:isValid() end

-- ============================================================
--  TransferElementBase  (base for all accessors)
-- ============================================================

---@class TransferElementBase
local TransferElementBase = {}

---Blocking read: waits until new data arrive.
function TransferElementBase:read() end

---Non-blocking read attempt. Returns true if new data were received.
---@return boolean
function TransferElementBase:readNonBlocking() end

---Read latest value, discarding stale data. Returns true if new data arrived.
---@return boolean
function TransferElementBase:readLatest() end

---Write the current value to the network.
function TransferElementBase:write() end

---Write, allowing the data to be lost if unread.
function TransferElementBase:writeDestructively() end

---@return string
function TransferElementBase:getName() end

---@return string
function TransferElementBase:getUnit() end

---@return string
function TransferElementBase:getDescription() end

---@return DataType
function TransferElementBase:getValueType() end

---@return VersionNumber
function TransferElementBase:getVersionNumber() end

---@return boolean
function TransferElementBase:isReadOnly() end

---@return boolean
function TransferElementBase:isReadable() end

---@return boolean
function TransferElementBase:isWriteable() end

---@return TransferElementID
function TransferElementBase:getId() end

---@return DataValidity
function TransferElementBase:dataValidity() end

-- ============================================================
--  ScalarAccessor
-- ============================================================

---@class ScalarAccessor : TransferElementBase
local ScalarAccessor = {}

---Return the current scalar value as a Lua primitive (number / string / boolean).
---@return any
function ScalarAccessor:get() end

---Blocking read, then return the new scalar value.
---@return any
function ScalarAccessor:readAndGet() end

---Set the in-memory value (does NOT write to the network).
---@param value any
function ScalarAccessor:set(value) end

---Set the value and immediately write to the network.
---@param value any
function ScalarAccessor:setAndWrite(value) end

---Write to the network only if the new value differs from the current one.
---@param value any
function ScalarAccessor:writeIfDifferent(value) end

-- ============================================================
--  ArrayAccessor
-- ============================================================

---@class ArrayAccessor : TransferElementBase
local ArrayAccessor = {}

---Blocking read; returns self for chaining (`for i,v in pairs(arr:readAndGet())`).
---@return ArrayAccessor
function ArrayAccessor:readAndGet() end

---Number of elements in the array.
---@return integer
function ArrayAccessor:getNElements() end

-- Array element access via [] — uses __index / __newindex metamethods.
-- Indices are 1-based (Lua convention).

-- ============================================================
--  VoidAccessor
-- ============================================================

---@class VoidAccessor : TransferElementBase
local VoidAccessor = {}

-- ============================================================
--  ReadAnyGroup
-- ============================================================

---@class ReadAnyGroup
local ReadAnyGroup = {}

---@return ReadAnyGroup
function ReadAnyGroup.new() end

---Add an accessor to the group.
---@param accessor TransferElementBase
function ReadAnyGroup:add(accessor) end

---Blocking read from any member. Returns the ID of the element that was read.
---@return TransferElementID
function ReadAnyGroup:readAny() end

---Non-blocking poll. Returns the ID (or an invalid ID if nothing was ready).
---@return TransferElementID
function ReadAnyGroup:readAnyNonBlocking() end

---Block until the given element has been updated.
---@param id_or_accessor TransferElementID|TransferElementBase
function ReadAnyGroup:readUntil(id_or_accessor) end

---Finalise the group (no more add() after this).
function ReadAnyGroup:finalise() end

---Interrupt a blocked readAny().
function ReadAnyGroup:interrupt() end

-- ============================================================
--  MatchingMode  (for DataConsistencyGroup)
-- ============================================================

---@class MatchingMode
---@field none       MatchingMode
---@field exact      MatchingMode
---@field historized MatchingMode
MatchingMode = {}

-- ============================================================
--  DataConsistencyGroup
-- ============================================================

---@class DataConsistencyGroup
local DataConsistencyGroup = {}

---@param mode MatchingMode
---@return DataConsistencyGroup
function DataConsistencyGroup.new(mode) end

---Add an accessor to the consistency group.
---@param accessor TransferElementBase
---@param histLen? integer   history length (for historized mode)
function DataConsistencyGroup:add(accessor, histLen) end

---Check whether the given ID produced a consistent snapshot. Returns true if so.
---@param id TransferElementID
---@return boolean
function DataConsistencyGroup:update(id) end

---@return MatchingMode
function DataConsistencyGroup:getMatchingMode() end

-- ============================================================
--  Severity  (for logger)
-- ============================================================

---@class Severity
---@field trace   Severity
---@field debug   Severity
---@field info    Severity
---@field warning Severity
---@field error   Severity
Severity = {}

-- ============================================================
--  LoggerStream
-- ============================================================

---@class LoggerStream
local LoggerStream = {}

---Emit a log line.
---@param message string
function LoggerStream:log(message) end

-- ============================================================
--  VariableGroup
-- ============================================================

---@class VariableGroup
local VariableGroup = {}

---@return string
function VariableGroup:getName() end

function VariableGroup:readAll() end
function VariableGroup:readAllLatest() end
function VariableGroup:readAllNonBlocking() end
function VariableGroup:writeAll() end
function VariableGroup:writeAllDestructively() end

-- ============================================================
--  ModuleGroup
-- ============================================================

---@class ModuleGroup
local ModuleGroup = {}

---@return string
function ModuleGroup:getName() end

-- ============================================================
--  ApplicationModule
-- ============================================================

---@class ApplicationModule
local ApplicationModule = {}

---@return string
function ApplicationModule:getName() end

---@param includeReturnChannels? boolean
function ApplicationModule:readAll(includeReturnChannels) end

---@param includeReturnChannels? boolean
function ApplicationModule:readAllLatest(includeReturnChannels) end

---@param includeReturnChannels? boolean
function ApplicationModule:readAllNonBlocking(includeReturnChannels) end

---@param includeReturnChannels? boolean
function ApplicationModule:writeAll(includeReturnChannels) end

---@return VersionNumber
function ApplicationModule:getCurrentVersionNumber() end

---@param vn VersionNumber
function ApplicationModule:setCurrentVersionNumber(vn) end

---@return DataValidity
function ApplicationModule:getDataValidity() end

function ApplicationModule:incrementDataFaultCounter() end
function ApplicationModule:decrementDataFaultCounter() end

---@return integer
function ApplicationModule:getDataFaultCounter() end

function ApplicationModule:disable() end

-- New API: method-call accessor factories on ApplicationModule
---@param type DataType  @param name string  @param unit string  @param description string  @return ScalarAccessor
function ApplicationModule:ScalarPushInput(type, name, unit, description) end
---@param type DataType  @param name string  @param unit string  @param description string  @return ScalarAccessor
function ApplicationModule:ScalarPushInputWB(type, name, unit, description) end
---@param type DataType  @param name string  @param unit string  @param description string  @return ScalarAccessor
function ApplicationModule:ScalarPollInput(type, name, unit, description) end
---@param type DataType  @param name string  @param unit string  @param description string  @return ScalarAccessor
function ApplicationModule:ScalarOutput(type, name, unit, description) end
---@param type DataType  @param name string  @param unit string  @param description string  @return ScalarAccessor
function ApplicationModule:ScalarOutputPushRB(type, name, unit, description) end
---@param type DataType  @param name string  @param unit string  @param description string  @return ScalarAccessor
function ApplicationModule:ScalarOutputReverseRecovery(type, name, unit, description) end
---@param type DataType  @param name string  @param unit string  @param nElements integer  @param description string  @return ArrayAccessor
function ApplicationModule:ArrayPushInput(type, name, unit, nElements, description) end
---@param type DataType  @param name string  @param unit string  @param nElements integer  @param description string  @return ArrayAccessor
function ApplicationModule:ArrayPushInputWB(type, name, unit, nElements, description) end
---@param type DataType  @param name string  @param unit string  @param nElements integer  @param description string  @return ArrayAccessor
function ApplicationModule:ArrayPollInput(type, name, unit, nElements, description) end
---@param type DataType  @param name string  @param unit string  @param nElements integer  @param description string  @return ArrayAccessor
function ApplicationModule:ArrayOutput(type, name, unit, nElements, description) end
---@param type DataType  @param name string  @param unit string  @param nElements integer  @param description string  @return ArrayAccessor
function ApplicationModule:ArrayOutputPushRB(type, name, unit, nElements, description) end
---@param type DataType  @param name string  @param unit string  @param nElements integer  @param description string  @return ArrayAccessor
function ApplicationModule:ArrayOutputReverseRecovery(type, name, unit, nElements, description) end
---@param name string  @param description string  @return VoidAccessor
function ApplicationModule:VoidInput(name, description) end
---@param name string  @param description string  @return VoidAccessor
function ApplicationModule:VoidOutput(name, description) end
---@param name string  @param description string  @return VariableGroup
function ApplicationModule:VariableGroup(name, description) end
---@param name string  @param description string  @return StatusAccessor
function ApplicationModule:StatusOutput(name, description) end
---@param name string  @param description string  @return StatusAccessor
function ApplicationModule:StatusPushInput(name, description) end
---@param name string  @param description string  @return StatusAccessor
function ApplicationModule:StatusPollInput(name, description) end

-- ============================================================
--  StatusAccessor
-- ============================================================

---@class StatusAccessor : TransferElementBase
local StatusAccessor = {}

---Get current status as integer (use Status.OFF/OK/WARNING/FAULT constants).
---@return integer
function StatusAccessor:get() end

---Blocking read, then return the status integer.
---@return integer
function StatusAccessor:readAndGet() end

---Set the in-memory status value.
---@param val integer
function StatusAccessor:set(val) end

---Set and immediately write the status value.
---@param val integer
function StatusAccessor:setAndWrite(val) end

---Write only if the new value differs from the current one.
---@param val integer
function StatusAccessor:writeIfDifferent(val) end

-- ============================================================
--  Status enum (integer constants for StatusAccessor)
-- ============================================================

---@class Status
---@field OFF     integer
---@field OK      integer
---@field WARNING integer
---@field FAULT   integer
Status = {}

-- ============================================================
--  PriorityMode  (for StatusAggregator)
-- ============================================================

---@class PriorityMode
---@field fwok          PriorityMode
---@field fwko          PriorityMode
---@field fw_warn_mixed PriorityMode
---@field ofwk          PriorityMode
PriorityMode = {}

-- ============================================================
--  Global factory functions
-- ============================================================

---Create a new unique VersionNumber. Pass nil to get the null version.
---@param nil_arg? nil
---@return VersionNumber
function VersionNumber(nil_arg) end

---Emit a log line directly (new API).
---@param severity Severity
---@param context  string
---@param message  string
function log(severity, context, message) end

---Create a logger stream for the given severity and context (old API).
---@param severity Severity
---@param context  string
---@return LoggerStream
function logger(severity, context) end

-- ============================================================
--  ConfigReader (Lua appConfig() API)
-- ============================================================

---@class ConfigReader
local ConfigReader = {}

---Read a scalar value, type inferred from the Lua default value.
---@param path    string
---@param default any   (type inferred: number→float64, string→string, boolean→Boolean)
---@return any
function ConfigReader:get(path, default) end

---@param path    string
---@param default any[]  (type inferred from first element)
---@return any[]
function ConfigReader:getArray(path, default) end

---Return a list of sub-module names under the given path.
---@param path string
---@return string[]
function ConfigReader:getModules(path) end

---Obtain the application configuration reader.
---@return ConfigReader
function appConfig() end

---Create a ModuleGroup owned by a parent group (or app root if no owner given).
---@overload fun(owner: ModuleGroup, name: string, description: string): ModuleGroup
---@overload fun(name: string, description: string): ModuleGroup
---@param owner      ModuleGroup
---@param name        string
---@param description string
---@return ModuleGroup
function ModuleGroup(owner, name, description) end

---Create a VariableGroup owned by a parent module or group.
---@param owner      ModuleGroup|ApplicationModule|VariableGroup
---@param name        string
---@param description string
---@return VariableGroup
function VariableGroup(owner, name, description) end

-- New API: method-call accessor factories on VariableGroup
---@param type DataType  @param name string  @param unit string  @param description string  @return ScalarAccessor
function VariableGroup:ScalarPushInput(type, name, unit, description) end
---@param type DataType  @param name string  @param unit string  @param description string  @return ScalarAccessor
function VariableGroup:ScalarOutput(type, name, unit, description) end
---@param name string  @param description string  @return StatusAccessor
function VariableGroup:StatusOutput(name, description) end
---@param name string  @param description string  @return StatusAccessor
function VariableGroup:StatusPushInput(name, description) end
---@param name string  @param description string  @return StatusAccessor
function VariableGroup:StatusPollInput(name, description) end
---@param name string  @param description string  @return VariableGroup
function VariableGroup:VariableGroup(name, description) end

---Create an ApplicationModule owned by a ModuleGroup.
---@param owner       ModuleGroup
---@param name        string
---@param description string
---@return ApplicationModule
function ApplicationModule(owner, name, description) end

-- Extra module constructors
---Create a PeriodicTrigger module.
---@param owner ModuleGroup  @param name string  @param description string  @param defaultPeriod? integer
function PeriodicTrigger(owner, name, description, defaultPeriod) end

---Create a StatusAggregator module.
---@param owner ModuleGroup  @param outputName string  @param description string  @param mode? PriorityMode
function StatusAggregator(owner, outputName, description, mode) end

---Create a MaxMonitor module.
---@param type DataType  @param owner ModuleGroup  @param inputPath string  @param outputPath string  @param parameterPath string  @param description string
function MaxMonitor(type, owner, inputPath, outputPath, parameterPath, description) end

---Create a MinMonitor module.
---@param type DataType  @param owner ModuleGroup  @param inputPath string  @param outputPath string  @param parameterPath string  @param description string
function MinMonitor(type, owner, inputPath, outputPath, parameterPath, description) end

---Create a RangeMonitor module.
---@param type DataType  @param owner ModuleGroup  @param inputPath string  @param outputPath string  @param parameterPath string  @param description string
function RangeMonitor(type, owner, inputPath, outputPath, parameterPath, description) end

---Create an ExactMonitor module.
---@param type DataType  @param owner ModuleGroup  @param inputPath string  @param outputPath string  @param parameterPath string  @param description string
function ExactMonitor(type, owner, inputPath, outputPath, parameterPath, description) end

-- The root module group; set by LuaModuleManager before each script is executed.
---@type ModuleGroup
app = nil
