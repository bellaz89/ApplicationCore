-- testLuaDCGSender.lua
-- Companion to testLuaDCGReceiver.lua. Writes triplets of in1/in2/in3 with
-- controlled VersionNumbers so the receiver's DataConsistencyGroup can verify
-- consistent vs inconsistent updates. Mirrors the Sender in
-- testPythonDataConsistencyGroup.py.

local mod = ApplicationModule("UserModule", "DCG sender")

mod.out1 = mod:ScalarOutput(DataType.int32, "/UserModule/in1", "", "")
mod.out2 = mod:ScalarOutput(DataType.int32, "/UserModule/in2", "", "")
mod.out3 = mod:ScalarOutput(DataType.int32, "/UserModule/in3", "", "")

function mod:prepare()
  -- Seed initial values so the receiver's push inputs unblock at startup.
  self.out1:write()
  self.out2:write()
  self.out3:write()
end

function mod:mainLoop()
  -- Batch 1: all three written under a single VersionNumber. The receiver's
  -- DataConsistencyGroup.update should report consistent on the third update.
  self:setCurrentVersionNumber(VersionNumber())
  self.out1:write()
  self.out3:write()
  self.out2:write()

  -- Batch 2: in1+in3 at one version, in2 at a later version. No update should
  -- ever yield a consistent set in MatchingMode.exact.
  self:setCurrentVersionNumber(VersionNumber())
  self.out1:write()
  self.out3:write()
  self:setCurrentVersionNumber(VersionNumber())
  self.out2:write()
end

return mod
