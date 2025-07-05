#pragma once

#include <lua.hpp>
#include <lauxlib.h>
#include <string>
#include <iostream>
#include <typeinfo>
#include <type_traits>
#include <stdexcept>

// Generic function pointer for Lua C functions
typedef int (*lua_func)(lua_State *);

// ─────────────────────────────────────────────────────────────
// Pointer FFI: push/get pointers with optional GC
// ─────────────────────────────────────────────────────────────

template<typename T>
T* getPtr(lua_State* L, int index) {
    if (!lua_isuserdata(L, index)) {
        throw std::runtime_error("Expected userdata at index");
    }
    void* userdata = lua_touserdata(L, index);
    return static_cast<T*>(*reinterpret_cast<void**>(userdata));
}

template<typename T>
void pushPtr(lua_State* L, T* ptr, bool gc = true) {
    if (!ptr) {
        lua_pushnil(L);
        return;
    }

    void** userdata = reinterpret_cast<void**>(lua_newuserdata(L, sizeof(void*)));
    *userdata = ptr;

    if (gc) {
        const char* metatype = typeid(T).name();
        if (luaL_newmetatable(L, metatype)) {
            lua_pushstring(L, "__gc");
            lua_pushcfunction(L, [](lua_State* L) -> int {
                void* ud = lua_touserdata(L, 1);
                if (ud) delete static_cast<T*>(*reinterpret_cast<void**>(ud));
                return 0;
            });
            lua_settable(L, -3);
        }
        lua_setmetatable(L, -2);
    }
}

// ─────────────────────────────────────────────────────────────
// Table argument parsing helpers
// ─────────────────────────────────────────────────────────────

inline double getArgByName(lua_State* L, const char* key, int index) {
    if (!lua_istable(L, index)) {
        luaL_error(L, "Expected table at index %d", index);
        return 0;
    }
    lua_getfield(L, index, key);
    if (!lua_isnumber(L, -1)) {
        lua_pop(L, 1);
        luaL_error(L, "Expected number for field '%s'", key);
        return 0;
    }
    double value = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return value;
}

inline double getGlobalNumber(lua_State* L, const char* key) {
    lua_getglobal(L, key);
    if (!lua_isnumber(L, -1)) {
        lua_pop(L, 1);
        luaL_error(L, "Expected number global '%s'", key);
        return 0;
    }
    double value = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return value;
}

// ─────────────────────────────────────────────────────────────
// Lua module helpers
// ─────────────────────────────────────────────────────────────

inline void push_funcs(lua_State* L, luaL_Reg funcs[]) {
    for (int i = 0; funcs[i].name != nullptr; i++) {
        lua_pushcfunction(L, funcs[i].func);
        lua_setfield(L, -2, funcs[i].name);
    }
}

inline void newModule(const char* name, luaL_Reg funcs[], lua_State* L) {
    lua_newtable(L);
    push_funcs(L, funcs);
    lua_setglobal(L, name);
}
