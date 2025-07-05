#pragma once
#include <lua.hpp>

void initFuncs(lua_State* L);   // Registers both Vec2 and Vec3
void initVec2(lua_State* L);
void initVec3(lua_State* L);
