-- testLuaBoolArray.lua: tests ArrayPushInput / ArrayOutput with DataType.Boolean

local mod = ApplicationModule("BoolArray", "")

mod.boolIn    = mod:ArrayPushInput(DataType.Boolean, "BoolIn",  "", 4, "")
mod.boolOut   = mod:ArrayOutput(DataType.Boolean,   "BoolOut", "", 4, "")
mod.testError = mod:ScalarOutput(DataType.string,   "TestError", "", "")

function mod:mainLoop()
  -- Initial write: element-by-element via __index/__newindex (exercises getElement / setElement)
  for i = 1, #self.boolIn do
    self.boolOut[i] = not self.boolIn[i]
  end
  self.boolOut:write()

  local step = 0
  while true do
    self.boolIn:read()
    step = step + 1
    if step % 2 == 1 then
      -- Odd steps: element-by-element (getElement / setElement)
      for i = 1, #self.boolIn do
        self.boolOut[i] = not self.boolIn[i]
      end
    else
      -- Even steps: toTable / fromTable
      local t = self.boolIn:toTable()
      for i = 1, #t do
        t[i] = not t[i]
      end
      self.boolOut:fromTable(t)
    end
    self.boolOut:write()
  end
end

return mod
