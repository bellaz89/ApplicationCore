-- testLuaUpvalue.lua
-- Tests that file-scope locals (upvalues) are correctly migrated to module VM.

local cfg      = appConfig()
local addend   = cfg:get("Upvalue/addend",   10.0)
local prefix   = cfg:get("Upvalue/prefix",   "v=")
local enabled  = cfg:get("Upvalue/enabled",  true)

local mod = ApplicationModule("Upvalue", "Upvalue migration test")

local input  = mod:ScalarPushInput(DataType.float32, "input",  "V", "Input")
local output = mod:ScalarOutput   (DataType.float32, "output", "V", "Output")
local label  = mod:ScalarOutput   (DataType.string,  "label",  "",  "Label")

function mod.mainLoop(self)
    -- addend, prefix, enabled, input, output, label are all upvalues
    if not enabled then
        output:setAndWrite(0.0)
        label:setAndWrite("disabled")
        return
    end
    output:setAndWrite(0.0)
    label:setAndWrite(prefix .. "init")
    while true do
        local v = input:readAndGet()
        output:setAndWrite(v + addend)
        label:setAndWrite(prefix .. tostring(v + addend))
    end
end
