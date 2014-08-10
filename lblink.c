#include "lua.h"
#include "lauxlib.h"
#include "blink1-lib.h"
#include "lblink.h"

// NOTE: we are only dealing with the Mk2 (which was called Mk1 on the website)
// (the actual mk1 was the first kickstarter version)


// TODO: implement an "all" ID to turn them all of, set them all to red, etc
// TODO: implement an "all" LED # to affect them both ??


static const char *BLINK_TYPENAME = "Blink1";
static const char *VID_KEY = "VID";
static const char *PID_KEY = "PID";
static const char *VERSION_KEY = "_VERSION";

static const char *VERSION = "0.5.1";

typedef struct blinker {
  blink1_device *device;
} blinker;




/************************************************************************************
 *
 * Methods
 *
 ************************************************************************************/

static int lfun_close(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  blink1_close(bd->device);
  bd->device = NULL;

  return 0;
}




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




// TODO: support setting only 1 of the LEDs
// however this isn't supported by the blink library so we 
// need to think about how best to do this (possibly use fade
// w/a very short time)
// FIXME: why can I call this with missing args?
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


SET(On, 255, 255, 255)
SET(Off, 0, 0, 0)
SET(Black, 0, 0, 0)
SET(White, 255, 255, 255)
SET(Red, 255, 0, 0)
SET(Green, 0, 255, 0)
SET(Blue, 0, 0, 255)
SET(Cyan, 0, 255, 255)
SET(Magenta, 255, 0, 255)
SET(Yellow, 255, 255, 0)




static int lfun_fadeToRGB(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  int millis = lua_tointeger(L, 2);
  int r = lua_tointeger(L, 3);
  int g = lua_tointeger(L, 4);
  int b = lua_tointeger(L, 5);
  int nLed = luaL_optint(L, 6, 0);

  int result = blink1_fadeToRGBN(bd->device, millis, r, g, b, nLed);

  if (!result) {
    lua_pushboolean(L, 1);
  } else {
    lua_pushnil(L);
  }
  return 1;
}




static int lfun_readRGB(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  int nLed = luaL_optint(L, 2, 0);

  uint16_t millis;
  uint8_t r, g, b;

  int result = blink1_readRGB(bd->device, &millis, &r, &g, &b, nLed);

  if (!result) {
    lua_pushinteger(L, r);
    lua_pushinteger(L, g);
    lua_pushinteger(L, b);
    lua_pushinteger(L, millis);
    return 4;
  } else {
    lua_pushnil(L);
    return 1;
  }
}




static int lfun_sleep(lua_State *L)
{
  int millis = luaL_optint(L, 2, 100);
  blink1_sleep(millis);
  return 0;
}




/************************************************************************************
 *
 * Functions
 *
 ************************************************************************************/

static int lfun_nDevices(lua_State *L)
{
  lua_pushnumber(L, blink1_enumerate());
  return 1;
}




static int lfun_list(lua_State *L)
{
  return 0;
}




// TODO: pass optional index
static int lfun_open(lua_State *L)
{
  int devid = luaL_optint(L, 1, 0);
  int nDevices = blink1_enumerate();

  // TODO: check that 0 <= devid < nDevices

  blinker *b = (blinker *)lua_newuserdata(L, sizeof(blinker));
  b->device = NULL;

  luaL_getmetatable(L, BLINK_TYPENAME);
  lua_setmetatable(L, -2);

  b->device = blink1_openById(devid);
  if (b->device == NULL) {
    // TODO: remove/release the userdatum
    // TODO: include device id in error message
    luaL_error(L, "could not open blink1");
  }
  
  return 1;
}




/************************************************************************************
 *
 * Entry point for library
 *
 ************************************************************************************/

static const luaL_Reg lblink_methods[] = {
  {"firmware", lfun_firmwareVersion},
  {"isMk2", lfun_isMk2},

  {"set", lfun_setRGB},
  {"on", lfun_setOn},
  {"off", lfun_setOff},
  {"black", lfun_setBlack},
  {"white", lfun_setWhite},
  {"red", lfun_setRed},
  {"green", lfun_setGreen},
  {"blue", lfun_setBlue},
  {"cyan", lfun_setCyan},
  {"magenta", lfun_setMagenta},
  {"yellow", lfun_setYellow},

  {"fade", lfun_fadeToRGB}, 
  {"read", lfun_readRGB}, 
  // {"serverdown", lfun_serverdown},

  // play and loop should both make use of blink1_playloop
  // and in fact, loop can call play
  // pass all the params in a table instead of positionally?

  // {"play", lfun_play},
  // {"loop", lfun_playloop},
  // {"stop", lfun_stop},

  // {"readplay", lfun_readplay},
  // {"writepatternline", lfun_writePatternLine},
  // {"readpatternline", lfun_readPatternLine},
  // {"savepattern", lfun_savePattern},

  {"sleep", lfun_sleep},
  {"__gc", lfun_close},
  {NULL, NULL}
};


static const luaL_Reg lblink_functions[] = {
  {"nDevices", lfun_nDevices},
  {"list", lfun_list}, // return a table describing all attached devices
  {"open", lfun_open},
  {NULL, NULL}
};




int luaopen_lblink(lua_State *L)
{
  luaL_newmetatable(L, BLINK_TYPENAME);

  // metatable.__index = metatable
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);

  luaL_setfuncs(L, lblink_methods, 0);
  // should we now drop the metatable?
  luaL_newlib(L, lblink_functions);

  lua_pushnumber(L, blink1_vid());
  lua_setfield(L, -2, VID_KEY);

  lua_pushnumber(L, blink1_pid());
  lua_setfield(L, -2, PID_KEY);
  
  lua_pushstring(L, VERSION);
  lua_setfield(L, -2, VERSION_KEY);

  return 1;
}
