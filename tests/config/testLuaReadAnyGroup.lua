-- testLuaReadAnyGroup.lua
-- Lua port of testPythonReadAnyGroup.py: exercises ReadAnyGroup add/finalise,
-- readAny, readAnyNonBlocking, readUntil(accessor) and readUntil(id).

local mod = ApplicationModule("UserModule", "ReadAnyGroup test module")

mod.in1       = mod:ScalarPushInput(DataType.int32,  "in1",       "", "")
mod.in2       = mod:ArrayPushInput (DataType.int32,  "in2",       "", 4, "")
mod.in3       = mod:ScalarPushInput(DataType.int32,  "in3",       "", "")
mod.output    = mod:ScalarOutput   (DataType.string, "output",    "", "")
mod.testError = mod:ScalarOutput   (DataType.string, "testError", "", "")

local function check(cond, msg)
  if not cond then error(msg or "assertion failed", 2) end
end

function mod:mainLoop()
  local ok, err = pcall(function()
    -- ReadAnyGroup over two of the three inputs. in3 is intentionally outside.
    local group = ReadAnyGroup()
    group:add(self.in1)
    group:add(self.in2)
    group:finalise()

    local id = group:readAny()
    check(id == self.in1:getId(), "step1: expected in1 id")
    check(self.in1:get() == 12,   "step1: expected in1 == 12")
    self.output:setAndWrite("step1")

    id = group:readAny()
    check(id == self.in2:getId(), "step2: expected in2 id")
    for i = 1, 4 do
      check(self.in2[i] == 24, "step2: expected in2[" .. i .. "] == 24")
    end
    self.output:setAndWrite("step2")

    -- in3 is not part of the group; read it directly. No pending updates left in the group.
    check(self.in3:readAndGet() == 36,             "step3: expected in3 == 36")
    check(not group:readAnyNonBlocking():isValid(), "step3: expected no pending group updates")
    self.output:setAndWrite("step3")

    -- readUntil(accessor) waits until that specific accessor has been observed.
    group:readUntil(self.in1)
    self.output:setAndWrite("step4")

    group:readUntil(self.in2)
    self.output:setAndWrite("step5")

    -- readUntil with a TransferElementID form.
    group:readUntil(self.in1:getId())
    self.output:setAndWrite("step6")
  end)

  if not ok then
    local msg = tostring(err)
    self.testError:setAndWrite(msg)
    self.output:setAndWrite("error: " .. msg)
  end

  -- ChimeraTK expects mainLoop to run forever; idle here on a blocking read so the
  -- testable-mode scheduler treats this module as quiescent until shutdown.
  while true do
    self.in1:read()
  end
end

return mod
