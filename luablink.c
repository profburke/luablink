#include "lua.h"
#include "lauxlib.h"
#include "blink1-lib.h"
#include "luablink.h"

// NOTE: we are only dealing with the Mk2 (which was called Mk1 on the website)
// (the actual mk1 was the first kickstarter version)


static const char *BLINK_TYPENAME = "Blink1";
static const char *VID_KEY = "VID";
static const char *PID_KEY = "PID";
static const char *VERSION_KEY = "_VERSION";

static const char *VERSION = "0.1.1";

typedef struct blinker {
  blink1_device *device;
} blinker;



// NOTE: This isn't needed since all the functions that
// manipulate blinks are methods, they can't be called unless
// the first argument is a valid blink object...
// blinker *checkBlinker(lua_State *L, int index)
// {
//   void *ud = luaL_checkudata(L, index, BLINK_TYPENAME);
//   luaL_argcheck(L, ud != NULL, index, "'blink' expected");
//   return (blinker *)ud;
// }




/************************************************************************************
 *
 * Methods
 *
 ************************************************************************************/

static int lfun_firmwareVersion(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  lua_pushinteger(L, blink1_getVersion(bd->device));

  return 1;
}




static int lfun_isMk2(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  lua_pushboolean(L, blink1_isMk2(bd->device));

  return 1;
}




// TODO: make this user-callable as well?
static int lfun_close(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  blink1_close(bd->device);
  bd->device = NULL;
  
  fprintf(stderr, "close called");

  return 0;
}




static int lfun_setRGB(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  int r = lua_tointeger(L, 2);
  int g = lua_tointeger(L, 3);
  int b = lua_tointeger(L, 4);

  luaL_argcheck(L, ( r > -1 && r < 256), 2, 
                "red value must be in range [0, 255]");
  luaL_argcheck(L, ( g > -1 && g < 256), 3, 
                "green value must be in range [0, 255]");
  luaL_argcheck(L, ( b > -1 && b < 256), 4, 
                "blue value must be in range [0, 255]");

  int retval = blink1_setRGB(bd->device, r, g, b);

  if (retval == -1) {
    lua_pushnil(L);
    lua_pushstring(L, "Could not set RGB");
    return 2;
  } else {
    lua_pushboolean(L, 1);
    return 1;
  }
}


#define SET(Color, r, g, b) \
  static int lfun_set##Color(lua_State *L) \
  { \
    lua_settop(L, 1);        \
    lua_pushinteger(L, (r)); \
    lua_pushinteger(L, (g)); \
    lua_pushinteger(L, (b)); \
    return lfun_setRGB(L);   \
  }


SET(White, 255, 255, 255)

SET(On, 255, 255, 255)

SET(Off, 0, 0, 0)

SET(Black, 0, 0, 0)

SET(Red, 255, 0, 0)

SET(Green, 0, 255, 0)

SET(Blue, 0, 0, 255)

SET(Cyan, 0, 255, 255)

SET(Magenta, 255, 0, 255)

SET(Yellow, 255, 255, 0)



/************************************************************************************
 *
 * Functions
 *
 ************************************************************************************/

static int lfun_enumerate(lua_State *L)
{
  lua_pushnumber(L, blink1_enumerate());
  return 1;
}




// TODO: pass optional index
static int lfun_open(lua_State *L)
{
  blinker *b = (blinker *)lua_newuserdata(L, sizeof(blinker));
  b->device = NULL;

  luaL_getmetatable(L, BLINK_TYPENAME);
  lua_setmetatable(L, -2);

  b->device = blink1_open();
  // TODO: if blink1_open returns NULL, then error:  luaL_error(L, "could not open blink1...");

  return 1;
}





/************************************************************************************
 *
 * Entry point for library
 *
 ************************************************************************************/

static const luaL_Reg luablink_methods[] = {
  {"firmware", lfun_firmwareVersion},
  {"isMk2", lfun_isMk2},
  {"set", lfun_setRGB},
  {"red", lfun_setRed},
  {"green", lfun_setGreen},
  {"blue", lfun_setBlue},
  {"cyan", lfun_setCyan},
  {"magenta", lfun_setMagenta},
  {"yellow", lfun_setYellow},
  {"white", lfun_setWhite},
  {"on", lfun_setOn},
  {"black", lfun_setBlack},
  // {"fade", lfun_fadeToRGB}, // and fadeToRGBN
  // {"read", lfun_readRGB}, // and readRGBN
  // {"serverdown", lfun_serverdown},
  // {"play", lfun_play},
  // {"loop", lfun_playloop},
  // {"readplay", lfun_readplay},
  // {"writepatternline", lfun_writePatternLine},
  // {"readpatternline", lfun_readPatternLine},
  // {"savepattern", lfun_savePattern},
  // error_msg
  // testtest
  // sleep
  {"__gc", lfun_close},
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

  // metatable.__index = metatable
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);

  luaL_setfuncs(L, luablink_methods, 0); // should we now drop the metatable?
  luaL_newlib(L, luablink_functions);

  lua_pushnumber(L, blink1_vid());
  lua_setfield(L, -2, VID_KEY);

  lua_pushnumber(L, blink1_pid());
  lua_setfield(L, -2, PID_KEY);
  
  lua_pushstring(L, VERSION);
  lua_setfield(L, -2, VERSION_KEY);

  return 1;
}
