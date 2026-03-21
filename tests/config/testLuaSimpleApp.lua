-- testLuaSimpleApp.lua
-- Simple push-pull module: reads an int32 input, writes float output = input + 0.5

local mod = ApplicationModule(app, "SomeName", "Description", function(self)
  self.myOutput:setAndWrite(0.5)
  while true do
    local val = self.myInput:readAndGet()
    self.myOutput:setAndWrite(val + 0.5)
  end
end)

mod.myOutput = ScalarOutput   (DataType.float32, mod, "/Var1", "unit", "description")
mod.myInput  = ScalarPushInput(DataType.int32,   mod, "/Var2", "unit", "description")
