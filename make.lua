local lm = require 'luamake'

lm.mode = "debug"

lm:lua_library 'findlocal' {
   sources = "findlocal.cpp",
   ldflags = {"/EXPORT:luaopen_findlocal_core"},
   export_luaopen = false,
}

lm:build "test" {
   "$luamake", "lua", "test.lua",
   deps = { "findlocal" }
}
