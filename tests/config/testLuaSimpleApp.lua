-- testLuaSimpleApp.lua
-- Simple push-pull module: reads an int32 input, writes float output = input + 0.5

local mod = ApplicationModule(app, "SomeName", "Description")

mod.myOutput = mod:ScalarOutput(DataType.float32, "/Var1", "unit", "description")
mod.myInput  = mod:ScalarPushInput(DataType.int32, "/Var2", "unit", "description")

function mod:mainLoop()
  self.myOutput:setAndWrite(0.5)
  while true do
    local val = self.myInput:readAndGet()
    self.myOutput:setAndWrite(val + 0.5)
  end
end
