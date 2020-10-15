local findlocal = require "findlocal.core"
local undump = require "undump"

local version

local function readall(filename)
    local f = assert(io.open(filename))
    local data = f:read "a"
    f:close()
    return data
end

local function getproto(source)
    if source:sub(1,1) == "=" then
        error "invalid source format"
    end
    local content = source:sub(1,1) == "@"
        and readall(source:sub(2))
        or source
    local bin = string.dump(assert(load(content)))
    local cl, v = undump(bin)
    version = v
    assert(version >= 503)
    return cl.f
end

local function updatelineinfo(f)
    local abs = {}
    for _, v in ipairs(f.abslineinfo) do
        abs[v.pc] = v.line
    end
    local l = f.lineinfo
    local n = f.linedefined
    for i, d in ipairs(l) do
        if d == -128 then
            n = assert(abs[i-1])
        else
            n = n + d
        end
        l[i] = n
    end
end

local function getfuncline(f, pc)
    if pc == 0 then
        return f.linedefined
    end
    return f.lineinfo[pc]
end

local function dump(info, f, parent)
    local function insert(t, v)
        if not t[v.startline] then
            t[v.startline] = {v}
            return
        end
        t = t[v.startline]
        for i, o in ipairs(t) do
            if o.pc > v.pc then
                table.insert(t, i, v)
                return
            end
        end
        table.insert(t, v)
    end
    if version >= 504 then
        updatelineinfo(f)
    end
    local cache = {locvar={},upvalue={}}
    for i = 1, f.sizelocvars do
        local locvar = f.locvars[i]
        local startline = getfuncline(f, locvar.startpc)
        local endline = getfuncline(f, locvar.endpc)
        local v = {
            name = locvar.varname,
            pc = locvar.startpc,
            startline = startline,
            endline = endline,
            ref = {},
        }
        cache.locvar[i] = v
        insert(info, v)
    end
    if parent == nil then
        cache.upvalue[1] = {}
    else
        for i = 1, f.sizeupvalues do
            local upvalue = f.upvalues[i]
            if upvalue.instack ~= 0 then
                local locvar = parent.locvar[upvalue.idx+1]
                assert(locvar and locvar.name == upvalue.name)
                cache.upvalue[i] = locvar.ref
            else
                cache.upvalue[i] = parent.upvalue[upvalue.idx+1]
            end
            table.insert(cache.upvalue[i], {f.linedefined, f.lastlinedefined, i})
        end
    end
    for i = 1, f.sizep do
        dump(info, f.p[i], cache)
    end
end

local function dumplocvar(source)
    local proto = getproto(source)
    local locvar = {}
    dump(locvar, proto)
    return locvar
end

local function splitno(locate)
    local pos = locate:find "#"
    if pos then
        return locate:sub(1, pos-1), tonumber(locate:sub(pos+1))
    end
    return locate, 1
end

local function findref(source, locate, near)
    local locvar = dumplocvar(source)
    if near then
        local curn, curv
        for l, vars in pairs(locvar) do
            for n, var in ipairs(vars) do
                if var.name == locate and var.startline < near and var.endline >= near then
                    if not curv or (curv.startline < l or (curv.startline == l and curn < n)) then
                        curn = n
                        curv = var
                    end
                end
            end
        end
        return curv
    else
        local name, no = splitno(locate)
        if name:match "^%d" then
            local line = tonumber(name)
            if locvar[line] and locvar[line][no] then
                return locvar[line][no]
            end
        else
            for _, vars in pairs(locvar) do
                for _, var in ipairs(vars) do
                    if var.name == name then
                        if no == 1 then
                            return var
                        end
                        no = no - 1
                    end
                end
            end
        end
    end
end

local function pack(t)
    local s = {}
    for _, v in ipairs(t) do
        s[#s+1] = string.pack("iii", table.unpack(v))
    end
    return table.concat(s)
end

return function(source, locate, near)
    local v = findref(source, locate, near)
    if not v then
        return
    end
    return findlocal(source, pack(v.ref))
end
