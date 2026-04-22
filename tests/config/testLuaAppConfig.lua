-- testLuaAppConfig.lua
-- Tests the appConfig() binding.

local mod = ApplicationModule("UserModule", "Config test module")
mod.testError = mod:ScalarOutput(DataType.string, "testError", "", "")

function mod:mainLoop()
  local errMsg = ""
  local cfg = appConfig()

  local s = cfg:get("stringScalar", "")
  if s ~= "a string scalar" then
    errMsg = errMsg .. "stringScalar mismatch: " .. tostring(s) .. "; "
  end

  local f = cfg:get("floatScalar", 0.0)
  if math.abs(f - 815.4711) > 0.01 then
    errMsg = errMsg .. "floatScalar mismatch: " .. tostring(f) .. "; "
  end

  local arr = cfg:getArray("floatArray")
  if #arr ~= 3 then
    errMsg = errMsg .. "floatArray length mismatch: " .. tostring(#arr) .. "; "
  end

  local missing = cfg:get("doesNotExist", 42.0)
  if missing ~= 42.0 then
    errMsg = errMsg .. "default value mismatch: " .. tostring(missing) .. "; "
  end

  self.testError:setAndWrite(errMsg)
end
