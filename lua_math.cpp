// funcs.cpp - functions that are useful in game development
#include "lua_math.hpp"
#include <cmath>
#include <lua.hpp>

// Function to create a new 2D vector in Lua
int newVec2(lua_State* ctx) {
    lua_newtable(ctx);
    lua_pushnumber(ctx, luaL_checknumber(ctx, 1)); // Ensure the first argument is a number
    lua_setfield(ctx, -2, "x");
    lua_pushnumber(ctx, luaL_checknumber(ctx, 2)); // Ensure the second argument is a number
    lua_setfield(ctx, -2, "y");
    return 1;
}

// Function to add two 2D vectors in Lua
int addVec2(lua_State* ctx) {
    // Ensure we have two arguments, both of which are tables
    luaL_checktype(ctx, 1, LUA_TTABLE);
    luaL_checktype(ctx, 2, LUA_TTABLE);

    // Get the x and y values from the first vector
    lua_getfield(ctx, 1, "x");
    double x1 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 1, "y");
    double y1 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    // Get the x and y values from the second vector
    lua_getfield(ctx, 2, "x");
    double x2 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 2, "y");
    double y2 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    // Create a new vector for the result
    lua_newtable(ctx);
    lua_pushnumber(ctx, x1 + x2);
    lua_setfield(ctx, -2, "x");
    lua_pushnumber(ctx, y1 + y2);
    lua_setfield(ctx, -2, "y");

    return 1;
}

int subVec2(lua_State* ctx) {
	// Ensure we have two arguments, both of which are tables
	luaL_checktype(ctx, 1, LUA_TTABLE);
	luaL_checktype(ctx, 2, LUA_TTABLE);

	// Get the x and y values from the first vector
	lua_getfield(ctx, 1, "x");
	double x1 = luaL_checknumber(ctx, -1);
	lua_pop(ctx, 1);

	lua_getfield(ctx, 1, "y");
	double y1 = luaL_checknumber(ctx, -1);
	lua_pop(ctx, 1);

	// Get the x and y values from the second vector
	lua_getfield(ctx, 2, "x");
	double x2 = luaL_checknumber(ctx, -1);
	lua_pop(ctx, 1);

	lua_getfield(ctx, 2, "y");
	double y2 = luaL_checknumber(ctx, -1);
	lua_pop(ctx, 1);

	// Create a new vector for the result
	lua_newtable(ctx);
	lua_pushnumber(ctx, x1 - x2);
	lua_setfield(ctx, -2, "x");
	lua_pushnumber(ctx, y1 - y2);
	lua_setfield(ctx, -2, "y");
	return 1;
}

int fromVec2ToRadians(lua_State* ctx) {
    // Ensure we have one argument, which is a table
    luaL_checktype(ctx, 1, LUA_TTABLE);

    // Get the x and y values from the vector
    lua_getfield(ctx, 1, "x");
    double x = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 1, "y");
    double y = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    // Convert the vector to radians

    lua_pushnumber(ctx, atan2(y, x));
    return 1;
}

int fromRadiansToVec2(lua_State* ctx) {
    // Ensure we have one argument, which is a number (angle in radians)
    double radians = luaL_checknumber(ctx, 1);

    // Convert the radians to a vector
    double x = cos(radians);
    double y = sin(radians);

    // Create a new vector for the result
    lua_newtable(ctx);
    lua_pushnumber(ctx, x);
    lua_setfield(ctx, -2, "x");
    lua_pushnumber(ctx, y);
    lua_setfield(ctx, -2, "y");

    return 1;
}


int newVec3(lua_State* ctx) {
    lua_newtable(ctx);
    lua_pushnumber(ctx, luaL_checknumber(ctx, 1)); // Ensure the first argument is a number
    lua_setfield(ctx, -2, "x");
    lua_pushnumber(ctx, luaL_checknumber(ctx, 2)); // Ensure the second argument is a number
    lua_setfield(ctx, -2, "y");
    lua_pushnumber(ctx, luaL_checknumber(ctx, 3)); // Ensure the third argument is a number
    lua_setfield(ctx, -2, "z");
    return 1;
}
int addVec3(lua_State* ctx) {
    // Ensure we have two arguments, both of which are tables
    luaL_checktype(ctx, 1, LUA_TTABLE);
    luaL_checktype(ctx, 2, LUA_TTABLE);

    // Get the x, y, and z values from the first vector
    lua_getfield(ctx, 1, "x");
    double x1 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 1, "y");
    double y1 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 1, "z");
    double z1 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    // Get the x, y, and z values from the second vector
    lua_getfield(ctx, 2, "x");
    double x2 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 2, "y");
    double y2 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 2, "z");
    double z2 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    // Create a new vector for the result
    lua_newtable(ctx);
    lua_pushnumber(ctx, x1 + x2);
    lua_setfield(ctx, -2, "x");
    lua_pushnumber(ctx, y1 + y2);
    lua_setfield(ctx, -2, "y");
    lua_pushnumber(ctx, z1 + z2);
    lua_setfield(ctx, -2, "z");

    return 1;
}

int subVec3(lua_State* ctx) {
    // Ensure we have two arguments, both of which are tables
    luaL_checktype(ctx, 1, LUA_TTABLE);
    luaL_checktype(ctx, 2, LUA_TTABLE);

    // Get the x, y, and z values from the first vector
    lua_getfield(ctx, 1, "x");
    double x1 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 1, "y");
    double y1 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 1, "z");
    double z1 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    // Get the x, y, and z values from the second vector
    lua_getfield(ctx, 2, "x");
    double x2 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 2, "y");
    double y2 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 2, "z");
    double z2 = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    // Create a new vector for the result
    lua_newtable(ctx);
    lua_pushnumber(ctx, x1 - x2);
    lua_setfield(ctx, -2, "x");
    lua_pushnumber(ctx, y1 - y2);
    lua_setfield(ctx, -2, "y");
    lua_pushnumber(ctx, z1 - z2);
    lua_setfield(ctx, -2, "z");

    return 1;
}

// Function to get the length of a 3D vector
int vec3Length(lua_State* ctx) {
    // Ensure we have one argument, which is a table
    luaL_checktype(ctx, 1, LUA_TTABLE);

    // Get the x, y, and z values from the vector
    lua_getfield(ctx, 1, "x");
    double x = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 1, "y");
    double y = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    lua_getfield(ctx, 1, "z");
    double z = luaL_checknumber(ctx, -1);
    lua_pop(ctx, 1);

    // Calculate the length of the vector
    double length = sqrt(x * x + y * y + z * z);

    // Push the length onto the Lua stack
    lua_pushnumber(ctx, length);

    return 1;
}

void initVec3(lua_State* L) {
    lua_newtable(L);
    lua_pushcfunction(L, newVec3);
    lua_setfield(L, -2, "new");
    lua_pushcfunction(L, addVec3);
    lua_setfield(L, -2, "add");
    lua_pushcfunction(L, subVec3);
    lua_setfield(L, -2, "sub");
    lua_pushcfunction(L, vec3Length);
    lua_setfield(L, -2, "length");
    lua_setglobal(L, "Vec3");
}

// Function to register the C++ functions in Lua
void initVec2(lua_State* L) {
    lua_newtable(L);
    lua_pushcfunction(L, newVec2);
    lua_setfield(L, -2, "new");
    lua_pushcfunction(L, addVec2);
    lua_setfield(L, -2, "add");
    lua_pushcfunction(L, subVec2);
    lua_setfield(L, -2, "sub");
    lua_pushcfunction(L, fromVec2ToRadians);
    lua_setfield(L, -2, "fromVec2ToRadians");
    lua_pushcfunction(L, fromRadiansToVec2);
    lua_setfield(L, -2, "fromRadiansToVec2");

    lua_setglobal(L, "Vec2");
}

void initFuncs(lua_State* L) {
    initVec2(L);
    initVec3(L);
}
