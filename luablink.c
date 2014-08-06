#include "lua.h"
#include "lauxlib.h"
#include "blink1-lib.h"
#include "luablink.h"


const char *BLINK_TYPENAME = "Blink1";


typedef struct blinker {
  blink1_device *device;
} blinker;


// gcc -DUSE_HIDAPI -bundle -undefined dynamic_lookup -o luablink.so luablink.c -lBlink1


static int lfun_setRGB(lua_State *L)
{
  blinker *bd = lua_touserdata(L, -4);
  int r = (int)lua_tonumber(L, -3);
  int g = (int)lua_tonumber(L, -2);
  int b = (int)lua_tonumber(L, -1);

  int retval = blink1_setRGB(bd->device, r, g, b);

  if (retval == -1) {
    lua_pushnil(L);
    lua_pushstring(L, "Could not set RGB");
    return 2;
  } else {
    return 1;
  }
}




static int lfun_enumerate(lua_State *L)
{
  lua_pushnumber(L, blink1_enumerate());
  return 1;
}




static int lfun_open(lua_State *L)
{
  blinker *b = (blinker *)lua_newuserdata(L, sizeof(blinker));

  luaL_getmetatable(L, BLINK_TYPENAME);
  lua_setmetatable(L, -2);

  b->device = blink1_open();

  return 1;
}




static const luaL_Reg luablink_methods[] = {
  {"setRGB", lfun_setRGB},
  {NULL, NULL}
};




static const luaL_Reg luablink_functions[] = {
  {"enumerate", lfun_enumerate},
  {"open", lfun_open},
  {NULL, NULL}
};




int luaopen_luablink(lua_State *L)
{
  luaL_newmetatable(L, BLINK_TYPENAME);

  // Set metatable.__index = metatable
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);

  luaL_setfuncs(L, luablink_methods, 0); // do I need to pop the metatable?
  luaL_newlib(L, luablink_functions);

  return 1;
}
