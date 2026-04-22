-- testLuaBadReturn.lua
-- Test that returning something other than an ApplicationModule throws an error

local mod = ApplicationModule("BadModule", "This will return something else")

-- Don't return the module - return a string instead
return "not a module"
