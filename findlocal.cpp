#include <lua.hpp>
#include <unordered_set>

class finder {
public:
    void initialize(lua_State* L);
    int  close(lua_State* L);
    bool alreadyVisited(lua_State* L);
    int  matchUpvalue(lua_State* L);
    void findUpvalue(lua_State* L);
private:
    struct upvalue {
        int linedefined;
        int lastlinedefined;
        int n;
    };
    const char*                     m_source = nullptr;
    const struct upvalue*           m_data = nullptr;
    size_t                          m_n = 0;
    int                             m_top = 0;
    std::unordered_set<const void*> m_visited;
    std::unordered_set<const void*> m_found;
};
finder g_finder;

void finder::initialize(lua_State* L) {
    size_t sz = 0;
    m_source = luaL_checkstring(L, 1);
    m_data = (const struct upvalue*)luaL_checklstring(L, 2, &sz);
    m_n = sz / sizeof(struct upvalue);
    m_top = 0;
}

int finder::close(lua_State* L) {
    m_visited.clear();
    m_found.clear();
    lua_settop(L, m_top);
    return m_top;
}

bool finder::alreadyVisited(lua_State* L) {
    return m_visited.insert(lua_topointer(L, -1)).second;
}

int finder::matchUpvalue(lua_State* L) {
    lua_Debug ar;
    lua_pushvalue(L, -1);
    if (lua_getinfo(L, ">S", &ar)) {
        if (strcmp(ar.source, m_source) == 0) {
            for (size_t i = 0; i < m_n; ++i) {
                if (m_data[i].linedefined == ar.linedefined && m_data[i].lastlinedefined == ar.lastlinedefined) {
                    return m_data[i].n;
                }
            }
        }
    }
    return 0;
}

void finder::findUpvalue(lua_State* L) {
    int n = matchUpvalue(L);
    if (!n) {
        return;
    }
    if (!lua_getupvalue(L, -1, n)) {
        return;
    }
    if (m_found.insert(lua_topointer(L, -1)).second) {
        lua_insert(L, ++m_top);
    }
    else {
        lua_pop(L, 1);
    }
}

static bool alreadyVisited(lua_State* L) {
    return g_finder.alreadyVisited(L);
}

static void visitObject(lua_State* L);

static void visitTable(lua_State* L) {
    if (!alreadyVisited(L)) {
        lua_pop(L, 1);
        return;
    }
    if (lua_getmetatable(L, -1)) {
        visitTable(L);
    }
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        visitObject(L);
        lua_pushvalue(L,-1);
        visitObject(L);
    }
    lua_pop(L,1);
}

static void visitUserdata(lua_State* L) {
    if (!alreadyVisited(L)) {
        lua_pop(L, 1);
        return;
    }
    if (lua_getmetatable(L, -1)) {
        visitTable(L);
    }
#if LUA_VERSION_NUM >= 504
    for (int i = 1;; ++i) {
        if (lua_getiuservalue(L, -1, i) == LUA_TNONE) {
            lua_pop(L, 1);
            break;
        }
        visitObject(L);
    }
#else
    lua_getuservalue(L, -1);
    visitObject(L);
#endif
    lua_pop(L, 1);
}

static void visitFunction(lua_State* L) {
    if (!alreadyVisited(L)) {
        lua_pop(L, 1);
        return;
    }
    g_finder.findUpvalue(L);
    for (int i = 1;; i++) {
        const char* name = lua_getupvalue(L, -1, i);
        if (name == NULL)
            break;
        visitObject(L);
    }
    lua_pop(L, 1);
}

static void visitThread(lua_State* L) {
    if (!alreadyVisited(L)) {
        lua_pop(L, 1);
        return;
    }
    int level = 0;
    lua_State* cL = lua_tothread(L, -1);
    if (cL == L) {
        level = 1;
    }
    else {
        int top = lua_gettop(cL);
        luaL_checkstack(cL, 1, NULL);
        for (int i = 0; i < top; i++) {
            lua_pushvalue(cL, i+1);
            visitObject(cL);
        }
    }
    lua_Debug ar;
    while (lua_getstack(cL, level, &ar)) {
        for (int j = 1; j > -1; j -= 2) {
            for (int i = j;; i += j) {
                const char* name = lua_getlocal(cL, &ar, i);
                if (name == NULL)
                    break;
                visitObject(cL);
            }
        }
        ++level;
    }
    lua_pop(L,1);
}

static void visitObject(lua_State* L) {
    luaL_checkstack(L, LUA_MINSTACK, NULL);
    switch (lua_type(L, -1)) {
    case LUA_TTABLE: visitTable(L); break;
    case LUA_TUSERDATA: visitUserdata(L);  break;
    case LUA_TFUNCTION: visitFunction(L); break;
    case LUA_TTHREAD: visitThread(L); break;
    default: lua_pop(L, 1); break;
    }
}

static int findLocal(lua_State* L) {
    g_finder.initialize(L);
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    visitTable(L);
    return g_finder.close(L);
}

extern "C" 
#if defined(_WIN32)
__declspec(dllexport)
#endif
int luaopen_findlocal_core(lua_State* L) {
    luaL_checkversion(L);
    lua_pushcfunction(L, findLocal);
    return 1;
}
