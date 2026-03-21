-- testLuaWithArray.lua
-- Tests ArrayPushInput / ArrayOutput view semantics and iteration.
-- Pattern mirrors testPythonWithArray.py.

local mod = ApplicationModule(app, "SomeName", "Module's description", function(self)
  -- Initial output: use arrayIn1 buffer value (set via setArrayDefault before runApplication)
  local sum1 = 0
  for i = 1, #self.arrayIn1 do
    sum1 = sum1 + self.arrayIn1[i]
  end
  for i = 1, #self.arrayOut1 do
    self.arrayOut1[i] = sum1 + (i - 1)
  end
  self.arrayOut1:write()

  while true do
    -- Blocking read of arrayIn2; readAndGet() returns self, iterate with pairs
    local sum2 = 0
    for _, v in pairs(self.arrayIn2:readAndGet()) do
      sum2 = sum2 + v
    end
    for i = 1, #self.arrayOut2 do
      self.arrayOut2[i] = sum2 + (i - 1)
    end
    self.arrayOut2:write()

    -- Test view-write: modify element in-place and read it back
    local saved = self.arrayIn1[1]
    self.arrayIn1[1] = 999
    local ok = (self.arrayIn1[1] == 999)
    self.arrayIn1[1] = saved  -- restore before the next read

    if not ok then
      self.testError:setAndWrite("view write: element[1] roundtrip failed")
    end

    -- Blocking read of arrayIn1; compute sum and write arrayOut1
    self.arrayIn1:read()
    sum1 = 0
    for i = 1, #self.arrayIn1 do
      sum1 = sum1 + self.arrayIn1[i]
    end
    for i = 1, #self.arrayOut1 do
      self.arrayOut1[i] = sum1 + (i - 1)
    end
    self.arrayOut1:write()
  end
end)

mod.arrayOut1  = ArrayOutput   (DataType.int32, mod, "ArrayOut1", "SomeUnit", 10, "my fancy description")
mod.arrayOut2  = ArrayOutput   (DataType.int32, mod, "ArrayOut2", "SomeUnit", 10, "my fancy description")
mod.arrayIn1   = ArrayPushInput(DataType.int32, mod, "ArrayIn1",  "unit",      2, "description")
mod.arrayIn2   = ArrayPushInput(DataType.int32, mod, "ArrayIn2",  "unit",      5, "description")
mod.testError  = ScalarOutput  (DataType.string, mod, "TestError", "", "")
