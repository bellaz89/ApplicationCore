-- testLuaFillTable.lua: tests ArrayAccessor:fillTable(t [, n]) and that
-- toTable() still works correctly after the refactor.

local mod = ApplicationModule("FillTable", "")

mod.arrIn  = mod:ArrayPushInput(DataType.int32, "ArrIn",  "", 5, "")
mod.arrOut = mod:ArrayOutput(DataType.int32,   "ArrOut", "", 5, "")
-- testError carries a non-empty string on any assertion failure.
mod.testError = mod:ScalarOutput(DataType.string, "TestError", "", "")

function mod:mainLoop()
  -- Pre-allocate a reusable table (simulates the hot-loop DAQ pattern).
  local buf = {}

  while true do
    self.arrIn:read()

    -- 1. fillTable with no second arg: should fill all 5 elements.
    self.arrIn:fillTable(buf)
    local ok = true
    for i = 1, 5 do
      if buf[i] ~= self.arrIn[i] then ok = false end
    end
    if not ok then
      self.testError:setAndWrite("fillTable(t) mismatch")
      self.arrOut:write()
    end

    -- 2. fillTable with k=3: only first 3 elements should be overwritten.
    --    Poison positions 4 and 5 first, then verify they survive.
    local poison = {}
    for i = 1, 5 do poison[i] = buf[i] end
    poison[4] = -999
    poison[5] = -999
    self.arrIn:fillTable(poison, 3)
    -- Positions 1-3 must match arrIn; 4 and 5 must still be -999.
    for i = 1, 3 do
      if poison[i] ~= self.arrIn[i] then ok = false end
    end
    if poison[4] ~= -999 or poison[5] ~= -999 then ok = false end
    if not ok then
      self.testError:setAndWrite("fillTable(t,3) partial-fill mismatch")
      self.arrOut:write()
    end

    -- 3. toTable() must produce identical results to fillTable on a fresh table.
    local fresh = self.arrIn:toTable()
    for i = 1, 5 do
      if fresh[i] ~= buf[i] then ok = false end
    end
    if not ok then
      self.testError:setAndWrite("toTable() vs fillTable() mismatch")
      self.arrOut:write()
    end

    -- Echo input to output (double each element) so the test can verify round-trip.
    for i = 1, 5 do
      self.arrOut[i] = buf[i] * 2
    end
    self.arrOut:write()
  end
end

return mod
