/*** Lua binding to the Blink1 library.
 *
 * The main function in this library allows you to create Lua object bound to a blink(1) 
 * so you can control it with Lua code.
 *
 * NOTE: This library only implements the Mk 2 versions of the various functions.
 *
 * @module blink
 * @author Matthew M. Burke <matthew@bluedino.net>
 * @copyright 2014-2015 BlueDino Software
 * @license MIT License (see LICENSE file)
 *
 */
#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "blink1-lib.h"
#include "blink.h"

#define BLINK1_ERR (-1)
#define PATTERNPLAY_START 1
#define PATTERNPLAY_STOP 0

// @fixme why are these #defineS and the others are static constS?
#define BADDEVSPEC_MSG "Must be either an integer in [0, n-1] (where n is the number of attached devices) or a valid serial number."
#define BADDEVID_MSG "ID must be in [0,n-1] where n is the number of attached devices."
#define NODEV_MSG "No blink(1) devices attached."
#define IDOPENERR_MSG "Could not open blink(1) with id %d."
#define SERIALOPENERR_MSG "Could not open blink(1) with serial #%s."

static const char *BLINK_TYPENAME = "net.bluedino.Blink1";
static const char *VID_KEY = "VID";
static const char *PID_KEY = "PID";
static const char *VERSION_KEY = "_VERSION";
static const char *DEVID_KEY = "devid";
static const char *SERIALNUM_KEY = "serial";
static const char *MARK_KEY = "mark";
static const int DEFAULT_PAUSE = 100;

const char *LUABLINK_VERSION = "0.6.1";

typedef struct blinker {
  blink1_device *device;
} blinker;




/*
 * NOTE: Most of the Blink1 library functions _claim_ to return -1 on error, and 0 on
 *       success. This doesn't seem to be true. As best I can tell, they do all return -1
 *       on error; but testing (on OS X 10.9.4) shows most (I didn't test all) of the functions
 *       return 9 on success.
 *
 *       On a related note, given the way the blink1 and hid code is written, it is not currently
 *       possible to retrieve meaningful error codes/messages on error. So we are left knowing
 *       only that an error occured. It's not clear to me that knowing more specifics would be
 *       useful, however.
 *
 */




/************************************************************************************
 *
 * Functions
 *
 ************************************************************************************/

/*** Returns count of attached blink(1) devices.
 *
 * @function enumerate
 * @treturn int number of attached devices
 *
 */
static int lfun_enumerate(lua_State *L)
{
  lua_pushnumber(L, blink1_enumerate());
  return 1;
}




/*** Returns a table with descriptions of all attached devices.
 *
 * @function list
 * @treturn table each entry describes an attached device
 *
 */
static int lfun_list(lua_State *L)
{
  char buf[256];

  int nDevices = blink1_enumerate();
  lua_createtable(L, nDevices, 0);

  for (int i = 0; i < nDevices; i++) {

    // push (list) index for the row we're creating
    lua_pushinteger(L, i + 1);

    lua_createtable(L, 0, 3);

    lua_pushstring(L, DEVID_KEY);
    lua_pushinteger(L, i);
    lua_settable(L, -3);

    lua_pushstring(L, SERIALNUM_KEY);
    lua_pushstring(L, blink1_getCachedSerial(i));
    lua_settable(L, -3);

    lua_pushstring(L, MARK_KEY);
    lua_pushinteger(L, (blink1_isMk2ById(i)?2:1) );
    lua_settable(L, -3);

    // now put row into list
    lua_settable(L, -3);
  }

  return 1;
}




/*** Opens a blink(1) device.
 *
 * Create a userdata bound to a particular device. If this function
 * is called without a parameter, it returns a userdata bound to the first device it finds.
 *
 * Otherwise it attempts to bind to the specified device. A particular device is specified
 * either by an integer ID or a string of 8 hexadecimal characters.
 * The integer ID must be between <code>0</code> and <code>n-1</code> (<em>where n is the number of 
 * attached devices</em>). The hex string is the serial number of a particular device.
 *
 * @function open
 * @tparam[opt] ?string|int devspec device ID or serial number
 * @treturn userdata object bound to a device
 * @raise error if TBD
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
    return luaL_error(L, NODEV_MSG);
  }

  // @fixme I'm still not happy with the parameter validation
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
      return luaL_error(L, IDOPENERR_MSG, devid);
    } else {
      return luaL_error(L, SERIALOPENERR_MSG, serial);
    }
  }

  luaL_getmetatable(L, BLINK_TYPENAME);
  lua_setmetatable(L, -2);
  
  return 1;
}




/*** Platform-independent millisecond-resolution sleep.
 *
 * @function sleep
 * @tparam[opt] int millis number of milliseconds to pause; if not
 * specified, defaults to 100
 *
 */
static int lfun_sleep(lua_State *L)
{
  // TODO: validate millis
  int millis = luaL_optinteger(L, 1, DEFAULT_PAUSE);
  blink1_sleep(millis);
  return 0;
}




/****************************************************************************/


/*** The Blink(1) device class.
 *
 * @type Device
 *
 */

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




static int lfun_tostring(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  
  if (bd->device == NULL) {
    lua_pushstring(L, "blink(1): disconnected");
  } else {
    // @fixme getSerialForDev can return NULL
    const char *serial = blink1_getSerialForDev(bd->device);
    lua_pushfstring(L, "blink(1): [%s]", serial);
  }

  return 1;
}




/*** Returns the firmware version of the device.
 * The version number is returned as a scaled integer,
 * e.g. "v1.1" -> 101
 *
 * @function version
 * @treturn int firmware version number
 *
 */
static int lfun_firmwareVersion(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  // @fixme -- seems to always return an int, even on error
  //           can we test and see if there's a distinguished error value we
  //           can check for?
  lua_pushinteger(L, blink1_getVersion(bd->device));

  return 1;
}




/*** Returns the serial number of the device.
 *
 * @function serial
 * @treturn string serial number
 *
 */
static int lfun_serialNumber(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  // @fixme getSerialForDev can return NULL
  lua_pushstring(L, blink1_getSerialForDev(bd->device));

  return 1;
}




/*** Returns true/false if the device is/isn't a Mark 2.
 *
 * @function isMk2
 * @treturn boolean true if device is Mark 2, false otherwise
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
 * @tparam int r &mdash; red value in range [0-255]
 * @tparam int g &mdash; green value in range [0-255]
 * @tparam int b &mdash; blue value in range [0-255]
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

  int result = blink1_setRGB(bd->device, r, g, b);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
    lua_pushstring(L, "Could not set RGB");
    return 2;
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


/// Turns the device on. Equivalent to <code>set(255, 255, 255)</code>
// @function on
SET(On, 255, 255, 255)

/// Turns the device off. Equivalent to <code>set(0, 0, 0)</code>
// @function off
SET(Off, 0, 0, 0)

/// Turns the device off. Equivalent to <code>set(0, 0, 0)</code>
// @function black
SET(Black, 0, 0, 0)

/// Turns the device on. Equivalent to <code>set(255, 255, 255)</code>
// @function white
SET(White, 255, 255, 255)

/// Turns the device red. Equivalent to <code>set(255, 0, 0)</code>
// @function red
SET(Red, 255, 0, 0)

/// Turns the device green. Equivalent to <code>set(0, 255, 0)</code>
// @function green
SET(Green, 0, 255, 0)

/// Turns the device blue. Equivalent to <code>set(0, 0, 255)</code>
// @function blue
SET(Blue, 0, 0, 255)

/// Turns the device cyan. Equivalent to <code>set(0, 255, 255)</code>
// @function cyan
SET(Cyan, 0, 255, 255)

/// Turns the device magenta. Equivalent to <code>set(255, 0, 255)</code>
// @function magenta
SET(Magenta, 255, 0, 255)

/// Turns the device yellow. Equivalent to <code>set(255, 255, 0)</code>
// @function yellow
SET(Yellow, 255, 255, 0)

/// Turns the device orange. Equivalent to <code>set(255, 165, 0)</code>
// @function orange
SET(Orange, 255, 165, 0)




/*** Fades device to given RGB over given number of milliseconds.
 *
 * @function fade
 * @int millis the fade duration
 * @int red the red component [0-255]
 * @int green the green component [0-255]
 * @int blue the blue component [0-255]
 * @int n which LED to adjust; 0 - both; 1 - top; 2 - bottom
 */
static int lfun_fadeToRGB(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  // TODO: validate millis, r, g, b, nLed
  int millis = lua_tointeger(L, 2);
  int r = lua_tointeger(L, 3);
  int g = lua_tointeger(L, 4);
  int b = lua_tointeger(L, 5);
  int nLed = luaL_optinteger(L, 6, 0);

  int result = blink1_fadeToRGBN(bd->device, millis, r, g, b, nLed);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
    lua_pushstring(L, "Could not fade to ....");
    return 2;
  }
}




/*** Returns the last RGB value for the given device.
 *
 * @function read
 * @treturn int last set red value [0, 255]
 * @treturn int last set green value [0, 255]
 * @treturn int last set blue value [0, 255]
 * @treturn int last set millis value
 *
 */
static int lfun_readRGB(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  // TODO: validate nLed
  int nLed = luaL_optinteger(L, 2, 0);

  uint16_t millis;
  uint8_t r, g, b;

  int result = blink1_readRGB(bd->device, &millis, &r, &g, &b, nLed);

  if (result != BLINK1_ERR) {
    lua_pushinteger(L, r);
    lua_pushinteger(L, g);
    lua_pushinteger(L, b);
    lua_pushinteger(L, millis);
    return 4;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
    lua_pushstring(L, "could not retrieve rgb");
    return 2;
  }
}




/// Plays the current pattern.
// @function play
static int lfun_play(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  // TODO: validate startpos, endpos, count
  uint8_t startpos = luaL_optinteger(L, 2, 0);
  uint8_t endpos = luaL_optinteger(L, 3, 0);
  uint8_t count = luaL_optinteger(L, 4, 0);

  int result = blink1_playloop(bd->device, PATTERNPLAY_START, startpos, endpos, count);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
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

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
    lua_pushstring(L, "Error stopping play.");
    return 2;
  }
}




/// Retrieves information on current play state.
// @function readplay
static int lfun_readplay(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);

  uint8_t playing;
  uint8_t playstart;
  uint8_t playend;
  uint8_t playcount;
  uint8_t playpos;

  int result = blink1_readPlayState(bd->device, &playing, &playstart,
                                    &playend, &playcount, &playpos);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, playing);
    lua_pushinteger(L, playstart);
    lua_pushinteger(L, playend);
    lua_pushinteger(L, playcount);
    lua_pushinteger(L, playpos);
    return 5;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
    lua_pushstring(L, "Could not retrieve playstate.");
    return 2;
  }
}




/// Write ...
// @function writepatternline
static int lfun_writePatternLine(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  // TODO: validate millis, r, g, b, pos
  uint16_t millis = luaL_checkinteger(L, 2);
  uint8_t r = luaL_checkinteger(L, 3);
  uint8_t g = luaL_checkinteger(L, 4);
  uint8_t b = luaL_checkinteger(L, 5);
  uint8_t pos = luaL_checkinteger(L, 6);

  int result = blink1_writePatternLine(bd->device, millis, r, g, b, pos);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
    // TODO: add pos to error message
    lua_pushstring(L, "Could not write pattern line");
    return 2;
  }
}




/// Read ...
// @function readpatternline
static int lfun_readPatternLine(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);
  // TODO: validate pos
  uint8_t pos = luaL_checkinteger(L, 2);

  uint16_t millis;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  int result = blink1_readPatternLine(bd->device, &millis, &r, &g, &b, pos);

  if (result != BLINK1_ERR) {
    lua_pushinteger(L, pos);
    lua_pushinteger(L, millis);
    lua_pushinteger(L, r);
    lua_pushinteger(L, g);
    lua_pushinteger(L, b);
    return 5;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
    // TODO: add pos to error message
    lua_pushstring(L, "Could not read pattern line");
    return 2;
  }
}




/// Save ...
// @function savepattern
static int lfun_savePattern(lua_State *L)
{
  blinker *bd = lua_touserdata(L, 1);

  int result = blink1_savePattern(bd->device);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    // TODO: move error message to define
    lua_pushstring(L, "Error saving pattern.");
    return 2;
  }
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
  {"version", lfun_firmwareVersion},
  {"serial", lfun_serialNumber},
  {"isMk2", lfun_isMk2},

  {"set", lfun_setRGB},
  // Is there a way to get these generated by the SET macro?
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
  {"readplay", lfun_readplay},

  {"writepatternline", lfun_writePatternLine},
  {"readpatternline", lfun_readPatternLine},
  {"savepattern", lfun_savePattern},

  {"close", lfun_close},
  {"__gc", lfun_close},
  {"__tostring", lfun_tostring},
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
LUABLINK_API int luaopen_blink(lua_State *L)
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
