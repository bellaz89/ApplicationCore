-- testLuaWithArray.lua
-- Tests ArrayPushInput / ArrayOutput view semantics and iteration.
-- Pattern mirrors testPythonWithArray.py.

local mod = ApplicationModule(app, "SomeName", "Module's description")

mod.arrayOut1 = mod:ArrayOutput(DataType.int32, "ArrayOut1", "SomeUnit", 10, "my fancy description")
mod.arrayOut2 = mod:ArrayOutput(DataType.int32, "ArrayOut2", "SomeUnit", 10, "my fancy description")
mod.arrayIn1  = mod:ArrayPushInput(DataType.int32, "ArrayIn1", "unit", 2, "description")
mod.arrayIn2  = mod:ArrayPushInput(DataType.int32, "ArrayIn2", "unit", 5, "description")
mod.testError = mod:ScalarOutput(DataType.string, "TestError", "", "")

function mod:mainLoop()
  local sum1 = 0
  for i = 1, #self.arrayIn1 do
    sum1 = sum1 + self.arrayIn1[i]
  end
  for i = 1, #self.arrayOut1 do
    self.arrayOut1[i] = sum1 + (i - 1)
  end
  self.arrayOut1:write()

  while true do
    local arr2 = self.arrayIn2:readAndGet()
    local sum2 = 0
    for i = 1, #arr2 do
      sum2 = sum2 + arr2[i]
    end
    for i = 1, #self.arrayOut2 do
      self.arrayOut2[i] = sum2 + (i - 1)
    end
    self.arrayOut2:write()

    local saved = self.arrayIn1[1]
    self.arrayIn1[1] = 999
    local ok = (self.arrayIn1[1] == 999)
    self.arrayIn1[1] = saved

    if not ok then
      self.testError:setAndWrite("view write: element[1] roundtrip failed")
    end

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
end
