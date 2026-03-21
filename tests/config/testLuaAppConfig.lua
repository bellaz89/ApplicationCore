-- testLuaAppConfig.lua
-- Tests the appConfig() ConfigReader binding.

local mod = ApplicationModule(app, "UserModule", "Config test module", function(self)
  local errMsg = ""
  local cfg = appConfig()

  local s = cfg:get(DataType.string, "stringScalar")
  if s ~= "a string scalar" then
    errMsg = errMsg .. "stringScalar mismatch: " .. tostring(s) .. "; "
  end

  local f = cfg:get(DataType.float64, "floatScalar")
  if math.abs(f - 815.4711) > 0.01 then
    errMsg = errMsg .. "floatScalar mismatch: " .. tostring(f) .. "; "
  end

  local arr = cfg:getArray(DataType.float64, "floatArray")
  if #arr ~= 3 then
    errMsg = errMsg .. "floatArray length mismatch: " .. tostring(#arr) .. "; "
  end

  -- test default value
  local missing = cfg:get(DataType.int32, "doesNotExist", 42)
  if missing ~= 42 then
    errMsg = errMsg .. "default value mismatch: " .. tostring(missing) .. "; "
  end

  self.testError:setAndWrite(errMsg)
end)

mod.testError = ScalarOutput(DataType.string, mod, "testError", "", "")
