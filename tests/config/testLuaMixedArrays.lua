-- testLuaMixedArrays.lua: tests array accessors for float32, float64, uint32, string

local mod = ApplicationModule("MixedArrays", "")

mod.f32In  = mod:ArrayPushInput(DataType.float32, "Float32In",  "", 3, "")
mod.f32Out = mod:ArrayOutput(DataType.float32,   "Float32Out", "", 3, "")
mod.f64In  = mod:ArrayPollInput(DataType.float64, "Float64In",  "", 3, "")
mod.f64Out = mod:ArrayOutput(DataType.float64,   "Float64Out", "", 3, "")
mod.u32In  = mod:ArrayPollInput(DataType.uint32,  "Uint32In",   "", 3, "")
mod.u32Out = mod:ArrayOutput(DataType.uint32,    "Uint32Out",  "", 3, "")
mod.strIn  = mod:ArrayPollInput(DataType.string,  "StringIn",   "", 3, "")
mod.strOut = mod:ArrayOutput(DataType.string,    "StringOut",  "", 3, "")

function mod:mainLoop()
  while true do
    self.f32In:read()      -- blocks; triggers one iteration
    self.f64In:readLatest()
    self.u32In:readLatest()
    self.strIn:readLatest()

    for i = 1, #self.f32Out do self.f32Out[i] = self.f32In[i] * 2.0   end
    for i = 1, #self.f64Out do self.f64Out[i] = self.f64In[i] + 1.0   end
    for i = 1, #self.u32Out do self.u32Out[i] = self.u32In[i] + 10    end
    for i = 1, #self.strOut do self.strOut[i] = self.strIn[i] .. "_out" end

    self.f32Out:write()
    self.f64Out:write()
    self.u32Out:write()
    self.strOut:write()
  end
end

return mod
