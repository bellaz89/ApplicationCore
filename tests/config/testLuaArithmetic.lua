-- testLuaArithmetic.lua
-- Tests arithmetic operators on scalar accessors.

local mod = ApplicationModule(app, "Arith", "Arithmetic metamethods test")

mod.a      = mod:ScalarPushInput(DataType.float32, "a",      "V", "Input A")
mod.b      = mod:ScalarPushInput(DataType.float32, "b",      "V", "Input B")
mod.sum    = mod:ScalarOutput   (DataType.float32, "sum",    "V", "a + b")
mod.diff   = mod:ScalarOutput   (DataType.float32, "diff",   "V", "a - b")
mod.prod   = mod:ScalarOutput   (DataType.float32, "prod",   "V", "a * b")
mod.quot   = mod:ScalarOutput   (DataType.float32, "quot",   "V", "a / b")
mod.neg    = mod:ScalarOutput   (DataType.float32, "neg",    "V", "-a")
mod.lt     = mod:ScalarOutput   (DataType.Boolean, "lt",     "",  "a < b")

function mod.mainLoop(self)
    self.a:read()
    self.b:read()
    self.sum:setAndWrite(self.a + self.b)
    self.diff:setAndWrite(self.a - self.b)
    self.prod:setAndWrite(self.a * self.b)
    self.quot:setAndWrite(self.a / self.b)
    self.neg:setAndWrite(-self.a)
    self.lt:setAndWrite(self.a < self.b)
    while true do
        self.a:read()
        self.b:read()
        self.sum:setAndWrite(self.a + self.b)
        self.diff:setAndWrite(self.a - self.b)
        self.prod:setAndWrite(self.a * self.b)
        self.quot:setAndWrite(self.a / self.b)
        self.neg:setAndWrite(-self.a)
        self.lt:setAndWrite(self.a < self.b)
    end
end
