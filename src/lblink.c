/*** Lua binding to the Blink1 library.
 * Lua binding to the blink(1) library. The main function in this library
 * allows you to create Lua object bound to a blink(1) so you can control
 * it with Lua code.
 *
 * NOTE: this library only implements the Mk 2 versions of the various functions.
 *
 * @module lblink
 * @author Matthew M. Burke <matthew@bluedino.net>
 * @copyright 2014 BlueDino Software
 *
 */
#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "blink1-lib.h"
#include "lblink.h"

#define PATTERNPLAY_START 1
#define PATTERNPLAY_STOP 0

#define BADDEVSPEC_MSG "Must be either an integer in [0, n-1] (where n is the number of attached devices) or a valid serial number."
#define BADDEVID_MSG "ID must be in [0,n-1] where n is the number of attached devices."

static const char *BLINK_TYPENAME = "net.bluedino.Blink1";
static const char *VID_KEY = "VID";
static const char *PID_KEY = "PID";
static const char *VERSION_KEY = "_VERSION";

const char *LUABLINK_VERSION = "0.6.1";

typedef struct blinker {
  blink1_device *device;
} blinker;




/************************************************************************************
 *
 * Methods
 *
 ************************************************************************************/


/*** Turns the device off, then closes it.
 *
 * @function close
 *
 */
static int lfun_close(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  blink1_setRGB(bd->device, 0, 0, 0);
  blink1_close(bd->device);
  bd->device = NULL;

  return 0;
}




/*** Returns the firmware version of the device.
 *
 * @function fwversion
 *
 */
static int lfun_firmwareVersion(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  lua_pushinteger(L, blink1_getVersion(bd->device));

  return 1;
}




/*** Returns the serial number of the device.
 *
 * @function serial
 *
 */
static int lfun_serialNumber(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  lua_pushstring(L, blink1_getSerialForDev(bd->device));

  return 1;
}




/*** Returns true/false if the device is/isn't a Mark 2.
 *
 * @function isMk2
 *
 */
static int lfun_isMk2(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  lua_pushboolean(L, blink1_isMk2(bd->device));

  return 1;
}




/*** Sets the device to the given RGB.
 *
 * @function set
 *
 */
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


/// Turns the blink(1) on (displaying white)
// @function on
SET(On, 255, 255, 255)

/// Turns the blink(1) off
// @function off
SET(Off, 0, 0, 0)

SET(Black, 0, 0, 0)
SET(White, 255, 255, 255)
SET(Red, 255, 0, 0)
SET(Green, 0, 255, 0)
SET(Blue, 0, 0, 255)
SET(Cyan, 0, 255, 255)
SET(Magenta, 255, 0, 255)
SET(Yellow, 255, 255, 0)
SET(Orange, 255, 165, 0)




/*** Fades device to given RGB over given number of milliseconds.
 *
 * @function fade
 *
 */
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




/*** Returns the last RGB value for the given device.
 *
 * @function read
 *
 */
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
    lua_pushstring(L, "could not retrieve rgb");
    return 2;
  }
}




/// Plays the current pattern.
// @function play
static int lfun_play(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);

  // TODO: optional arguments: startpos, endpos, count

  int result = blink1_playloop(bd->device, PATTERNPLAY_START, 0, 0, 0);

  if (!result) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "Error starting play.");
    return 2;
  }
}




/// Stops playing the current pattern.
// @function stop
static int lfun_stop(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);

  int result = blink1_playloop(bd->device, PATTERNPLAY_STOP, 0, 0, 0);

  if (!result) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "Error stopping play.");
    return 2;
  }
}




/************************************************************************************
 *
 * Functions
 *
 ************************************************************************************/

/*** Returns count of attached blink(1) devices.
 *
 * @function enumerate
 * @treturn int count of attached blink(1) devices
 *
 */
static int lfun_enumerate(lua_State *L)
{
  lua_pushnumber(L, blink1_enumerate());
  return 1;
}




// TODO: change this so that each entry is a table
// consisting of id, serial#, mk2/mk1
// there's a mis-match here since in Lua
// tables are (typically) 1-based...

/*** Returns a list of all attached blink(1) devices.
 *
 * @function list
 * @return table where each entry is information on an attached blink(1) device.
 *
 */
static int lfun_list(lua_State *L)
{
  char buf[256];

  int nDevices = blink1_enumerate();
  lua_createtable(L, nDevices, 0);

  for (int i = 0; i < nDevices; i++) {
    sprintf(buf, "serialnum: %s %s", 
            blink1_getCachedSerial(i), (blink1_isMk2ById(i)?"(mk2)":""));

    lua_pushinteger(L, i);
    lua_pushlstring(L, buf, strlen(buf));
    lua_settable(L, -3);
  }

  return 1;
}




/*** Opens a blink(1) device.
 *
 * Create a userdata bound to a particular blink(1) device. If this function
 * is called without a parameter, it returns a userdata bound to the first blink(1) it finds.
 *
 * Alternatively, you can specify either an integer ID or a string of 8 hexadecimal characters.
 * The integer ID must be in the range [0, n-1] (where n is the number of attached blink(1) devices).
 * The hex string is the serial number of a particular device.
 *
 * @function open
 * @tparam ?string|int devid optional device ID/serial number
 * @treturn userdata object bound to the specified blink(1) device.
 *
 */
static int lfun_open(lua_State *L)
{
  int devid = -1;
  char serial[9] = {'\0', '\0', '\0', 
                    '\0', '\0', '\0',
                    '\0', '\0', '\0'};


  int nDevices = blink1_enumerate();
  if (0 == nDevices) {
    return luaL_error(L, "No blink(1) devices attached.");
  }


  // I'm still not happy with the error handling
  if (0 == lua_gettop(L)) {
    devid = 0;
  } else if (lua_isnumber(L, 1)) {
    int argval = lua_tointeger(L, 1);
    if ( (-1 < argval) && (argval < nDevices) ) {
      devid = argval;
    } else {
      strncpy(serial, lua_tostring(L, 1), 8);
    }
  } else {
    return luaL_argerror(L, 1, BADDEVSPEC_MSG);
  }


  blinker *b = (blinker *)lua_newuserdata(L, sizeof(blinker));
  b->device = NULL;


  if (devid > -1) {
    b->device = blink1_openById(devid);
  } else {
    b->device = blink1_openBySerial(serial);
  }


  if (b->device == NULL) {
    if (devid > -1) {
      return luaL_error(L, "could not open blink(1) with id %d", devid);
    } else {
      return luaL_error(L, "could not open blink(1) with serial #%s", serial);
    }
  }


  luaL_getmetatable(L, BLINK_TYPENAME);
  lua_setmetatable(L, -2);

  
  return 1;
}




/*** Platform-independent millisecond-resolution sleep.
 *
 * @function sleep
 *
 */
static int lfun_sleep(lua_State *L)
{
  int millis = luaL_optint(L, 1, 100);
  blink1_sleep(millis);
  return 0;
}




/************************************************************************************
 *
 * Library Declaration
 *
 ************************************************************************************/


/*
 *
 * List of members on blink objects.
 *
 */
static const luaL_Reg lblink_methods[] = {
  {"fwversion", lfun_firmwareVersion},
  {"serial", lfun_serialNumber},
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
  {"orange", lfun_setOrange},
  {"fade", lfun_fadeToRGB}, 
  {"read", lfun_readRGB}, 
  {"play", lfun_play},
  {"stop", lfun_stop},

  // {"readplay", lfun_readplay},
  // {"writepatternline", lfun_writePatternLine},
  // {"readpatternline", lfun_readPatternLine},
  // {"savepattern", lfun_savePattern},

  {"close", lfun_close},
  {"__gc", lfun_close},
  {NULL, NULL}
};




/*
 *
 * List of library functions. We also set a few data values
 * in the open function.
 *
 */
static const luaL_Reg lblink_functions[] = {
  {"enumerate", lfun_enumerate},
  {"list", lfun_list}, 
  {"open", lfun_open},
  {"sleep", lfun_sleep},
  {NULL, NULL}
};



// DOES OFF NEED TO ALSO STOP PLAY OF PATTERN?



/*
 *
 * Main entry point for library.
 *
 * This function performs the following tasks:
 *
 * - create and populate the metatable for blink objects
 * - create and populate the library table
 *
 */
LUABLINK_API int luaopen_lblink(lua_State *L)
{
  luaL_newmetatable(L, BLINK_TYPENAME);

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
  
  lua_pushstring(L, LUABLINK_VERSION);
  lua_setfield(L, -2, VERSION_KEY);

  return 1;
}
