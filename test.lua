package.cpath = "./build/msvc/bin/?.dll"
local findlocal = require "findlocal"

local TEST_1 = "test-1"

local function foo()
    local TEST_0, TEST_1 = "", "test-2"
    return function ()
        return TEST_1
    end
end
local f = foo()

local SOURCE_TEST_LUA = debug.getinfo(1, "S").source
print(">", findlocal(SOURCE_TEST_LUA, "TEST_1"))
print(">", findlocal(SOURCE_TEST_LUA, "TEST_1#2"))
print(">", findlocal(SOURCE_TEST_LUA, "7"))
print(">", findlocal(SOURCE_TEST_LUA, "7#2"))
print(">", findlocal(SOURCE_TEST_LUA, "TEST_1", 5))
print(">", findlocal(SOURCE_TEST_LUA, "TEST_1", 9))
print(">", findlocal(SOURCE_TEST_LUA, "TEST_1", 12))
print "ok"
