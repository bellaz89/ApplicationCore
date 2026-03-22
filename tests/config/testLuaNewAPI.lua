-- testLuaNewAPI.lua
-- Tests the new proposal API: method-call accessors, upvalue migration,
-- xmlConfig.get(path, default), log(sev, ctx, msg)

local scale = xmlConfig.get("NewAPI/scale", 2.0)
local label = xmlConfig.get("NewAPI/label", "newapi")

local mod = ApplicationModule(app, "NewAPI", "New API test")

local input  = mod:ScalarPushInput(DataType.float32, "input",  "V", "Input")
local output = mod:ScalarOutput   (DataType.float32, "output", "V", "Output")
local status = mod:ScalarOutput   (DataType.string,  "status", "",  "Status")

function mod.mainLoop(self)
    -- upvalues: scale (number), label (string), input/output/status (userdata)
    log(Severity.info, label, "starting")
    output:setAndWrite(0.0)
    status:setAndWrite("ready")
    while true do
        local v = input:readAndGet()
        output:setAndWrite(v * scale)
        status:setAndWrite("ok")
    end
end
