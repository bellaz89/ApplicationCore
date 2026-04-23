-- testLuaDCGReceiver.lua
-- Companion to testLuaDCGSender.lua. Builds a ReadAnyGroup over the three inputs
-- plus a DataConsistencyGroup, then asserts the expected consistency outcomes
-- for the two batches the sender produces. Mirrors testPythonDataConsistencyGroup.py.

local mod = ApplicationModule("UserModule", "DCG receiver")

mod.in1       = mod:ScalarPushInput(DataType.int32,  "/UserModule/in1",       "", "")
mod.in2       = mod:ScalarPushInput(DataType.int32,  "/UserModule/in2",       "", "")
mod.in3       = mod:ScalarPushInput(DataType.int32,  "/UserModule/in3",       "", "")
mod.testError = mod:ScalarOutput   (DataType.string, "/UserModule/testError", "", "")

function mod:mainLoop()
  local ok, err = pcall(function()
    local rag = ReadAnyGroup()
    rag:add(self.in1)
    rag:add(self.in2)
    rag:add(self.in3)
    rag:finalise()

    local dcg = DataConsistencyGroup(MatchingMode.exact)
    dcg:add(self.in1)
    dcg:add(self.in2)
    dcg:add(self.in3)

    -- Batch 1: three writes under one VersionNumber. The third update completes
    -- the consistent set in MatchingMode.exact.
    local id = rag:readAny(); assert(not dcg:update(id), "batch1: 1st update unexpectedly consistent")
    id = rag:readAny();        assert(not dcg:update(id), "batch1: 2nd update unexpectedly consistent")
    id = rag:readAny();        assert(    dcg:update(id), "batch1: 3rd update should be consistent")

    -- Batch 2: in1 + in3 at one version, in2 at a later version. No update
    -- should yield a consistent set in MatchingMode.exact.
    id = rag:readAny(); assert(not dcg:update(id), "batch2: 1st update unexpectedly consistent")
    id = rag:readAny(); assert(not dcg:update(id), "batch2: 2nd update unexpectedly consistent")
    id = rag:readAny(); assert(not dcg:update(id), "batch2: 3rd update unexpectedly consistent")

    self.testError:setAndWrite("ok")
  end)

  if not ok then
    self.testError:setAndWrite(tostring(err))
  end

  -- Idle on a blocking read so testable mode treats us as quiescent until shutdown.
  while true do
    self.in1:read()
  end
end

return mod
