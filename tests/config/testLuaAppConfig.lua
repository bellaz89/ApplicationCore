-- testLuaAppConfig.lua
-- Tests the xmlConfig binding.

local mod = ApplicationModule(app, "UserModule", "Config test module")
mod.testError = mod:ScalarOutput(DataType.string, "testError", "", "")

function mod:mainLoop()
  local errMsg = ""

  local s = xmlConfig.get("stringScalar", "")
  if s ~= "a string scalar" then
    errMsg = errMsg .. "stringScalar mismatch: " .. tostring(s) .. "; "
  end

  local f = xmlConfig.get("floatScalar", 0.0)
  if math.abs(f - 815.4711) > 0.01 then
    errMsg = errMsg .. "floatScalar mismatch: " .. tostring(f) .. "; "
  end

  local arr = xmlConfig.getArray("floatArray")
  if #arr ~= 3 then
    errMsg = errMsg .. "floatArray length mismatch: " .. tostring(#arr) .. "; "
  end

  local missing = xmlConfig.get("doesNotExist", 42.0)
  if missing ~= 42.0 then
    errMsg = errMsg .. "default value mismatch: " .. tostring(missing) .. "; "
  end

  self.testError:setAndWrite(errMsg)
end
