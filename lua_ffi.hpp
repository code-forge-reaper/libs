#pragma once

#include <lua.hpp>
#include <string>
#include <type_traits>
#include <typeinfo>
#define FPANIC(fmt, ...)                                                       \
  do {                                                                         \
    fprintf(stderr, "%s:%i: " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);      \
    exit(1);                                                                   \
  } while (0)

#define PANIC(fmt)                                                             \
  do {                                                                         \
    fprintf(stderr, "%s:%i: " fmt "\n", __FILE__, __LINE__);                   \
    exit(1);                                                                   \
  } while (0)
// Generic function pointer for Lua C functions
using lua_func = int (*)(lua_State *);
#define newState(x)                                                            \
  lua_State *x = luaL_newstate();                                              \
  luaopen_base(x)
#define lfn static int
#define LUA_DEF(name, type, value, L)                                          \
  do {                                                                         \
    lua_push##type((L), value);                                                \
    lua_setfield((L), -2, (name));                                             \
  } while (0)

#define LUA_DEF_G(name, type, value, L)                                        \
  do {                                                                         \
    lua_push##type((L), value);                                                \
    lua_setglobal((L), (name));                                                \
  } while (0)

#define LUA_DEF_M(name, type, target, value, L)                                \
  do {                                                                         \
    lua_getglobal((L), (target));                                              \
    lua_push##type((L), value);                                                \
    lua_setfield((L), -2, (name));                                             \
    lua_pop((L), 1);                                                           \
  } while (0)

#define LUA_BEGIN_TABLE(L) lua_newtable(L);
#define LUA_END_TABLE(L) lua_pop(L, 1)

#define LUA_DEF_S(name, value, L) LUA_DEF(name, string, value, L)
#define LUA_DEF_I(name, value, L) LUA_DEF(name, integer, value, L)
#define LUA_DEF_N(name, value, L) LUA_DEF(name, number, value, L)
#define LUA_DEF_B(name, value, L) LUA_DEF(name, boolean, value, L)

// ----------------- GC wrapper for template types -----------------
template <typename T> static int lua_gc_wrapper(lua_State *L) {
  // userdata is at index 1
  void *ud = lua_touserdata(L, 1);
  if (!ud)
    return 0;
  T *p = *reinterpret_cast<T **>(ud);
  if (p) {
    delete p;
    // set pointer to nullptr to be extra-safe in case GC runs weirdly
    *reinterpret_cast<T **>(ud) = nullptr;
  }
  return 0;
}

// ----------------- register a metatable and methods once --------------
inline void register_metatable(lua_State *L, const char *metaname,
                               const luaL_Reg funcs[]) {
  // If metatable already exists in registry, just ensure __index points to
  // itself
  luaL_getmetatable(L, metaname); // pushes value or nil
  if (!lua_isnil(L, -1)) {
    // ensure __index is set to the metatable itself
    lua_pushvalue(L, -1);           // push metatable
    lua_setfield(L, -2, "__index"); // metatable.__index = metatable
    lua_pop(L, 1);                  // pop metatable
    return;
  }
  lua_pop(L, 1); // pop the nil

  // Create new metatable
  if (luaL_newmetatable(L, metaname) == 0) {
    // extremely unlikely: newmetatable failed to create but returned 0
    lua_pop(L, 1);
    luaL_error(L, "Failed to create metatable %s", metaname);
    return;
  }
  // stack: metatable

  // set functions on the metatable
#if LUA_VERSION_NUM >= 502
  luaL_setfuncs(L, funcs, 0);
#else
  for (int i = 0; funcs[i].name != nullptr; ++i) {
    lua_pushcfunction(L, funcs[i].func);
    lua_setfield(L, -2, funcs[i].name);
  }
#endif

  // metatable.__index = metatable
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");

  lua_pop(L, 1); // pop metatable
}

// ---------- push userdata (pointer) and attach existing metatable ----------
template <typename T>
void pushPtrWithMeta(lua_State *L, T *ptr, const char *metaname,
                     bool gc = true) {
  if (!ptr) {
    lua_pushnil(L);
    return;
  }

  // create userdata holding pointer (we store pointer value)
  void **ud = reinterpret_cast<void **>(lua_newuserdata(L, sizeof(void *)));
  *ud = ptr;
  // now stack: userdata

  // get metatable from registry (does not create)
  luaL_getmetatable(L, metaname); // pushes metatable or nil
  if (lua_isnil(L, -1)) {
    // no existing metatable -> create one
    lua_pop(L, 1);
    luaL_newmetatable(L, metaname); // pushes new metatable
    // make sure __index points to itself
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
  }
  // stack: userdata, metatable

  if (gc) {
    // attach GC function (overwrite if already present)
    lua_pushcfunction(L, &lua_gc_wrapper<T>);
    lua_setfield(L, -2, "__gc");
  }

  // set the metatable on userdata (this pops metatable)
  lua_setmetatable(L, -2); // userdata now has metatable attached
  // stack: userdata
}

// overload that uses typeid(T).name() as metatable name (fallback)
template <typename T> void pushPtr(lua_State *L, T *ptr, bool gc = true) {
  if (!ptr) {
    lua_pushnil(L);
    return;
  }

  void **userdata =
      reinterpret_cast<void **>(lua_newuserdata(L, sizeof(void *)));
  *userdata = ptr;

  const char *metatype = typeid(T).name();
  // luaL_newmetatable pushes the metatable on the stack and returns 1 if it
  // was created
  if (luaL_newmetatable(L, metatype)) {
    // newly created metatable
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    if (gc) {
      lua_pushcfunction(L, &lua_gc_wrapper<T>);
      lua_setfield(L, -2, "__gc");
    }
  } else {
    // metatable already existed; ensure __gc if requested
    if (gc) {
      lua_pushcfunction(L, &lua_gc_wrapper<T>);
      lua_setfield(L, -2, "__gc");
    }
  }
  // set metatable on userdata (pops metatable)
  lua_setmetatable(L, -2);
}

// convenience macro wrappers
#define ATTACH_TYPE(L, NAME, FUNCS) register_metatable((L), (NAME), (FUNCS))
#define PUSH_ATTACHED(L, T, PTR, NAME, GC)                                     \
  pushPtrWithMeta<T>((L), (PTR), (NAME), (GC))

// ---------------- Pointer FFI: push/get pointers with optional GC
// ----------------

template <typename T> T *getPtr(lua_State *L, int index) {
  if (!lua_isuserdata(L, index)) {
    luaL_error(L, "Expected userdata at index %d", index);
    return nullptr; // unreachable, but keeps compiler happy
  }
  void *userdata = lua_touserdata(L, index);
  if (!userdata) {
    luaL_error(L, "Invalid userdata (null) at index %d", index);
    return nullptr;
  }
  return static_cast<T *>(*reinterpret_cast<void **>(userdata));
}

// safer overload which checks metatable name (preferred when you have a stable
// metatable name)
template <typename T> T *getPtr(lua_State *L, int index, const char *metaname) {
  void **ud = reinterpret_cast<void **>(luaL_checkudata(L, index, metaname));
  if (!ud) {
    luaL_error(L,
               "Invalid userdata or wrong metatable at index %d (expected %s)",
               index, metaname);
    return nullptr;
  }
  return static_cast<T *>(*ud);
}

// non-throwing variant: returns nullptr instead of luaL_error
template <typename T> T *getRawPtr(lua_State *L, int index) noexcept {
  if (!lua_isuserdata(L, index))
    return nullptr;
  void *userdata = lua_touserdata(L, index);
  if (!userdata)
    return nullptr;
  return static_cast<T *>(*reinterpret_cast<void **>(userdata));
}

// ---------------- Table argument parsing helpers ----------------
// ---------------- Generic table argument parsing helpers ----------------

template <typename T> T getArgByName(lua_State *L, const char *key, int index) {
  if (!lua_istable(L, index)) {
    luaL_error(L, "Expected table at index %d", index);
    return T{};
  }

  lua_getfield(L, index, key);

  if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
    if (!lua_isnumber(L, -1)) {
      lua_pop(L, 1);
      luaL_error(L, "Expected number for field '%s'", key);
      return T{};
    }
    T value = static_cast<T>(lua_tonumber(L, -1));
    lua_pop(L, 1);
    return value;
  } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> ||
                       std::is_same_v<T, lua_Integer>) {
    if (!lua_isinteger(L, -1)) {
      lua_pop(L, 1);
      luaL_error(L, "Expected integer for field '%s'", key);
      return T{};
    }
    T value = static_cast<T>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return value;
  } else if constexpr (std::is_same_v<T, bool>) {
    if (!lua_isboolean(L, -1)) {
      lua_pop(L, 1);
      luaL_error(L, "Expected boolean for field '%s'", key);
      return T{};
    }
    T value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
  } else if constexpr (std::is_same_v<T, std::string> ||
                       std::is_same_v<T, const char *>) {
    if (!lua_isstring(L, -1)) {
      lua_pop(L, 1);
      luaL_error(L, "Expected string for field '%s'", key);
      return T{};
    }
    if constexpr (std::is_same_v<T, std::string>) {
      T value = lua_tostring(L, -1);
      lua_pop(L, 1);
      return value;
    } else {
      T value = lua_tostring(L, -1);
      lua_pop(L, 1);
      return value;
    }
  } else {
    static_assert(sizeof(T) == 0, "Unsupported type for getArgByName");
    return T{};
  }
}

template <typename T> T getGlobal(lua_State *L, const char *key) {
  lua_getglobal(L, key);

  if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
    if (!lua_isnumber(L, -1)) {
      lua_pop(L, 1);
      luaL_error(L, "Expected number global '%s'", key);
      return T{};
    }
    T value = static_cast<T>(lua_tonumber(L, -1));
    lua_pop(L, 1);
    return value;
  } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> ||
                       std::is_same_v<T, lua_Integer>) {
    if (!lua_isinteger(L, -1)) {
      lua_pop(L, 1);
      luaL_error(L, "Expected integer global '%s'", key);
      return T{};
    }
    T value = static_cast<T>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return value;
  } else if constexpr (std::is_same_v<T, bool>) {
    if (!lua_isboolean(L, -1)) {
      lua_pop(L, 1);
      luaL_error(L, "Expected boolean global '%s'", key);
      return T{};
    }
    T value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
  } else if constexpr (std::is_same_v<T, std::string> ||
                       std::is_same_v<T, const char *>) {
    if (!lua_isstring(L, -1)) {
      lua_pop(L, 1);
      luaL_error(L, "Expected string global '%s'", key);
      return T{};
    }
    if constexpr (std::is_same_v<T, std::string>) {
      T value = lua_tostring(L, -1);
      lua_pop(L, 1);
      return value;
    } else {
      T value = lua_tostring(L, -1);
      lua_pop(L, 1);
      return value;
    }
  } else {
    static_assert(sizeof(T) == 0, "Unsupported type for getGlobal");
    return T{};
  }
}

// Convenience macros for backward compatibility
#define getGlobalNumber(L, key) getGlobal<double>(L, key)
#define getGlobalString(L, key) getGlobal<const char *>(L, key)
#define getGlobalBoolean(L, key) getGlobal<bool>(L, key)
#define getGlobalInteger(L, key) getGlobal<lua_Integer>(L, key)

// Convenience macros for getArgByName
#define getArgNumber(L, key, index) getArgByName<double>(L, key, index)
#define getArgString(L, key, index) getArgByName<const char *>(L, key, index)
#define getArgBoolean(L, key, index) getArgByName<bool>(L, key, index)
#define getArgInteger(L, key, index) getArgByName<lua_Integer>(L, key, index)

// ---------------- Lua module helpers ----------------

inline void push_funcs(lua_State *L, const luaL_Reg funcs[]) {
  for (int i = 0; funcs[i].name != nullptr; i++) {
    lua_pushcfunction(L, funcs[i].func);
    lua_setfield(L, -2, funcs[i].name);
  }
}

// ---------- Small helpers for class "table" creation ----------
inline void create_class_table(lua_State *L, const char *luaName,
                               const luaL_Reg class_funcs[]) {
#if LUA_VERSION_NUM >= 502
  lua_createtable(L, 0, 0);         // push table
  luaL_setfuncs(L, class_funcs, 0); // set fields
#else
  lua_newtable(L);
  for (int i = 0; class_funcs[i].name != nullptr; ++i) {
    lua_pushcfunction(L, class_funcs[i].func);
    lua_setfield(L, -2, class_funcs[i].name);
  }
#endif
  lua_setglobal(L, luaName); // global LuaName = the table
}

// ---------- Primary macro (keeps your original convenience) ----------
#define LUA_CLASS(Type, LuaName, METHOD_INITS...)                              \
  static const luaL_Reg Type##_methods[] = {METHOD_INITS, {NULL, NULL}};       \
  static const luaL_Reg Type##_class[] = {{"new", Type##_new}, {NULL, NULL}};  \
  inline void register_##Type(lua_State *L) {                                  \
    ATTACH_TYPE(L, LuaName, (const luaL_Reg *)Type##_methods);                 \
    create_class_table(L, LuaName, (const luaL_Reg *)Type##_class);            \
  }

#define LUA_CLASS_AUTO(Type, METHOD_INITS...)                                  \
  LUA_CLASS(Type, #Type, METHOD_INITS)

inline void newModule(const char *name, const luaL_Reg funcs[], lua_State *L) {
  lua_newtable(L);
  push_funcs(L, funcs);
  lua_setglobal(L, name);
}
