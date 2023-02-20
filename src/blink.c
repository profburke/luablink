/*** Lua binding to the Blink1 library.
 *
 * Control blink(1) objects with Lua code.
 * (https://blink1.thingm.com/)
 *
 * NOTE: This library only implements the Mk 2 versions of the various functions.
 *
 * @module blink
 * @author Matthew M. Burke <matthew@bluedino.net>
 * @copyright 2014-2023 BlueDino Software
 * @license MIT License (see LICENSE file)
 *
 */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "lua.h"
#include "lauxlib.h"
#include "blink1-lib.h"
#include "blink.h"

#define BLINK1_ERR (-1)
#define PATTERNPLAY_START 1
#define PATTERNPLAY_STOP 0

// @fixme why are these #defineS and the others are static constS?
#define BADDEVSPEC_MSG "ID must be either an integer in [0, n-1] (n = number of attached blinks) or a valid serial number."
#define BADDEVID_MSG "ID must be in [0,n-1] (n = number of attached blinks)."
#define NODEV_MSG "No blink(1) devices attached."
#define IDOPENERR_MSG "Could not open blink(1) with id %d."
#define SERIALOPENERR_MSG "Could not open blink(1) with serial #%s."
#define BADSLEEPTIME_MSG "millis must be >= 0"
#define BADRED_MSG "red value must be in range [0, 255]"
#define BADGREEN_MSG "green value must be in range [0, 255]"
#define BADBLUE_MSG "blue value must be in range [0, 255]"
#define DISCONNECTED_BLINK_MSG "blink(1): disconnected"
#define BLINK_STRING_FMT "[blink(1) %s: #%s]"
#define BAD_RETRIEVAL_MSG "could not retrieve rgb"

static const char *BLINK_TYPENAME = "net.bluedino.Blink1";
static const char *VID_KEY = "VID";
static const char *PID_KEY = "PID";
static const char *VERSION_KEY = "_VERSION";
static const char *DEVID_KEY = "devid";
static const char *SERIALNUM_KEY = "serial";
static const char *MARK_KEY = "mark";
static const int DEFAULT_PAUSE = 100;

const char *LUABLINK_VERSION = "0.7.0";

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

/*** Returns ThingM's Vendor ID.
 *
 * @function vid
 * @treturn int ThingM's Vendor ID.
 *
 */
static int lfun_vid(lua_State *L) {
  lua_pushinteger(L, blink1_vid());
  
  return 1;
}

/*** Returns the blink(1) Product ID.
 *
 * @function pid
 * @treturn int blink(1) Product ID.
 *
 */
static int lfun_pid(lua_State *L) {
  lua_pushinteger(L, blink1_pid());
  
  return 1;
}

/*** Returns count of attached blink(1) devices.
 *
 * @function enumerate
 * @treturn int number of attached devices
 *
 */
static int lfun_enumerate(lua_State *L) {
  lua_pushinteger(L, blink1_enumerate());

  return 1;
}

/*** Returns a table with descriptions of all attached devices.
 *
 * @function list
 * @treturn table each entry describes an attached device
 *
 */
static int lfun_list(lua_State *L) {
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
    lua_pushinteger(L, blink1_deviceTypeById(i));
    lua_settable(L, -3);

    // now put row into list
    lua_settable(L, -3);
  }

  return 1;
}

/*** Opens a blink(1) device.
 *
 * Create a userdata bound to a specified blink(1). If this function is called without a parameter,
 * it returns a userdata bound to the first blink(1) it finds.
 *
 * Otherwise it attempts to bind to the specified device. A particular device is specified
 * either by an integer ID or a string of 8 hexadecimal characters. The integer ID must be between
 * <code>0</code> and <code>n-1</code> (<em>where n is the number of attached devices</em>). The hex string
 * is the serial number of a particular device.
 *
 * @function open
 * @tparam[opt] ?string|int devspec device ID or serial number
 * @treturn userdata object bound to the specified device
 * @raise error if TBD
 *
 */
static int lfun_open(lua_State *L) {
  int devid = -1;
  char serial[9] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};

  int nDevices = blink1_enumerate();
  if (0 == nDevices) {
    return luaL_error(L, NODEV_MSG);
  }

  // @fixme - not happy with this; clean it up and rethink
  if (0 == lua_gettop(L)) {
    devid = 0;
  } else if (lua_isinteger(L, 1)) {
    int argval = lua_tointeger(L, 1);
    if ( (-1 < argval) && (argval < nDevices) ) {
      devid = argval;
    } else {
      sprintf(serial, "%X", argval);
    }
  } else if (lua_isstring(L, 1)) {
    int isint;
    devid = lua_tointegerx(L, 1, &isint);
    if (!isint) {
      return luaL_error(L, BADDEVSPEC_MSG);
    }
  } else {
    return luaL_error(L, BADDEVSPEC_MSG);
  }

  blinker *b = (blinker *)lua_newuserdatauv(L, sizeof(blinker), 0);
  // No need to check that b is not null: if memory allocation failed,
  // we'd never return to here because the allocator throws an error.
  b->device = NULL;

  if (devid > -1) {
    b->device = blink1_openById(devid);
  } else {
    b->device = blink1_openBySerial(serial);
  }

  // I _think_ the userdata will be garbage collected since it doesn't get assigned.
  if (b->device == NULL) {
    char msg[100];
    if (devid > -1) {
      sprintf(msg, IDOPENERR_MSG, devid);
    } else {
      sprintf(msg, SERIALOPENERR_MSG, serial);
    }

    return luaL_error(L, msg);
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
static int lfun_sleep(lua_State *L) {
  int millis = luaL_optinteger(L, 1, DEFAULT_PAUSE);
  luaL_argcheck(L, (millis >= 0), 1, BADSLEEPTIME_MSG);
  blink1_sleep(millis);
  
  return 0;
}

/*** Disables gamma correction.
 *
 * @function noGamma
 *
 */
static int lfun_noDegamma(lua_State *L) {
  blink1_disableDegamma();

  return 0;
}

/*** Enables gamma correction.
 *
 * @function gamma
 *
 */
static int lfun_yesDegamma(lua_State *L) {
  blink1_enableDegamma();

  return 0;
}

// blink1_enable_degamma is static so can't access ...
// need to think of another approach...
//
// /*** Returns a boolean indicating whether gamma correction is enabled.
//  *
//  * @function gamma
//  * @treturn boolean true if gamma correction is enabled.
//  *
//  */
// static int lfun_isDegamma(lua_State *L) {
//   lua_pushboolean(L, blink1_enable_degamma);
  
//   return 1;
// }

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
static int lfun_close(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  blink1_setRGB(bd->device, 0, 0, 0);
  blink1_close(bd->device);
  bd->device = NULL;

  return 0;
}

static const char *getSerial(lua_State *L,blink1_device *device) {
  const char *serial = blink1_getSerialForDev(device);
  if (serial == NULL) {
    serial = "N/A";
  }
  lua_pushfstring(L, BLINK_STRING_FMT, serial);

  return serial;
}

static int lfun_tostring(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  
  if (bd->device == NULL) {
    lua_pushstring(L, DISCONNECTED_BLINK_MSG);
  } else {
    lua_pushfstring(L, BLINK_STRING_FMT,
                    blink1_deviceTypeToStr(blink1_deviceType(bd->device)),
                    getSerial(L, bd->device));
  }

  return 1;
}

/*** Returns the firmware version of the device.
 *
 * @function version
 * @treturn string firmware version number
 *
 */
static int lfun_firmwareVersion(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  // According to comments in C code, it seems that blink1_getVersion
  // _can_ return an error code, but what those error codes are is not
  // documented.
  int scaledVersion = blink1_getVersion(bd->device);
  int major = scaledVersion / 100;
  int minor = scaledVersion % 100;

  char buf[20];
  sprintf(buf, "%d%02d", major, minor);
  lua_pushstring(L, buf);

  return 1;
}

/*** Returns the serial number of the device.
 *
 * @function serial
 * @treturn string serial number
 *
 */
static int lfun_serialNumber(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  lua_pushstring(L, getSerial(L, bd->device));

  return 1;
}

/*** Returns true/false if the device is/isn't a Mark 2.
 *
 * @function isMk2
 * @treturn boolean true if device is Mark 2, false otherwise
 *
 */
static int lfun_isMk2(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  lua_pushboolean(L, blink1_isMk2(bd->device));

  return 1;
}

/*** Returns an integer indicating the version of the blink(1).
 *
 * @function type
 * @treturn in the version of the device
 *
 */
static int lfun_type(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  lua_pushinteger(L, blink1_deviceType(bd->device));

  return 1;
}

// TODO: type as string function

// TODO: document return value
/*** Sets the device to the given RGB.
 *
 * @function set
 * @tparam int r &mdash; red value in range [0-255]
 * @tparam int g &mdash; green value in range [0-255]
 * @tparam int b &mdash; blue value in range [0-255]
 *
 */
static int lfun_setRGB(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);

  int r = luaL_checkinteger(L, 2);
  int g = luaL_checkinteger(L, 3);
  int b = luaL_checkinteger(L, 4);
  
  luaL_argcheck(L, ( -1 < r && r < 256), 2, BADRED_MSG);
  luaL_argcheck(L, ( -1 < g && g < 256), 3, BADGREEN_MSG);
  luaL_argcheck(L, ( -1 < b && b < 256), 4, BADBLUE_MSG);

  int result = blink1_setRGB(bd->device, r, g, b);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "Could not set RGB");
    return 2;
  }
}

#define SET(Color, r, g, b) \
  static int lfun_set##Color(lua_State *L) { \
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

#define max(x, y) ( ((x) < (y)) ? (y) : (x) )

// TODO: need to specify which LED?
/*** Dims the color displayed by the device. Note: calling this function
 * forces Gamma correction off. You will need to explicitly turn it back on if
 * you want it afterwards.
 *
 * @function dim
 * @tresult boolean ...
 *
 */
static int lfun_dim(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  uint16_t millis;
  uint8_t r, g, b;

  int result = blink1_readRGB(bd->device, &millis, &r, &g, &b, 0);
  
  if (result == BLINK1_ERR) {
    lua_pushnil(L);
    lua_pushstring(L, "could not dim");
    return 2;
  }

  // We're going to force gamma correction off (and leave it off
  // since we can't (yet) check to see if it was on
  // Then we consider the range 0-255 broken up into 8 buckets,
  // and move to the next lower bucket (i.e. subtract 256/8)/

  blink1_disableDegamma();
  
  r = max(0, r - 32);
  g = max(0, g - 32);
  b = max(0, b - 32);
  result = blink1_setRGB(bd->device, r, g, b);
  
  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "could not dim");
    return 2;
  }
}

#define min(x, y) ( ((x) < (y)) ? (x) : (y) )

// TODO: need to specify which LED?
/*** Brightens the color displayed by the device. Note: calling this function
 * forces Gamma correction off. You will need to explicitly turn it back on if
 * you want it afterwards.
 *
 * @function brighten
 * @tresult boolean ...
 *
 */
static int lfun_brighten(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  uint16_t millis;
  uint8_t r, g, b;

  int result = blink1_readRGB(bd->device, &millis, &r, &g, &b, 0);

  if (result == BLINK1_ERR) {
    lua_pushnil(L);
    lua_pushstring(L, "could not read RGB values");
    return 2;
  }

  // Do the opposite of dim...
  blink1_disableDegamma();

  if (r != 0) { r = min(255, r + 32); }
  if (g != 0) { g = min(255, g + 32); }
  if (b != 0) { b = min(255, b + 32); }
  result = blink1_setRGB(bd->device, r, g, b);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "could not brighten");
    return 2;
  }
}

/*** Fades device to given RGB over given number of milliseconds.
 *
 * @function fade
 * @int millis the fade duration
 * @int red the red component [0-255]
 * @int green the green component [0-255]
 * @int blue the blue component [0-255]
 * @int n which LED to adjust; 0 - both; 1 - top; 2 - bottom
 */
static int lfun_fadeToRGB(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  int millis = luaL_checkinteger(L, 2);
  int r = luaL_checkinteger(L, 3);
  int g = luaL_checkinteger(L, 4);
  int b = luaL_checkinteger(L, 5);
  int nLed = luaL_optinteger(L, 6, 0);

  int result = blink1_fadeToRGBN(bd->device, millis, r, g, b, nLed);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);

    return 1;
  } else {
    char msg[256];
    sprintf(msg, "Could not fade to (%d, %d, %d)", r, g, b);

    lua_pushnil(L);
    lua_pushstring(L, msg);

    return 2;
  }
}

// TODO: deal with specifying which LED
// TODO: do we really need millis?
/*** Returns the last RGB value for the given device.
 *
 * @function get
 * @treturn int last set red value [0, 255]
 * @treturn int last set green value [0, 255]
 * @treturn int last set blue value [0, 255]
 * @treturn int last set millis value
 *
 */
static int lfun_readRGB(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  int nLed = luaL_optinteger(L, 2, 0);
  // TODO: is it 0|1 or 1|2 ?
  // luaL_argcheck(L, (nLed == 0 || nLed == 1), 1, "Led # must be 0 or 1.");
  
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
    lua_pushstring(L, BAD_RETRIEVAL_MSG);
    return 2;
  }
}

/*** Plays the current pattern. Optionally specify a sub-range to play, as
 * well as an optional count (0 = loop forever).
 *
 * @function play
 *
 */
static int lfun_play(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);

  int startpos = luaL_optinteger(L, 2, 0);
  int endpos = luaL_optinteger(L, 3, 0);
  int count = luaL_optinteger(L, 4, 0);

  // TODO: adjust upper bound based on whether mk1, mk2, etc.
  luaL_argcheck(L, ( -1 < startpos && startpos < 33), 2, "starting position must be in range [0, 32]");
  luaL_argcheck(L, ( -1 < endpos && endpos < 33), 3, "ending position must be in range [0, 32]");
  luaL_argcheck(L, ( startpos <= endpos ), 2, "start position must be before end position");
  luaL_argcheck(L, ( count > -1), 4, "count must be non-negative");
  
  int result = blink1_playloop(bd->device, PATTERNPLAY_START, startpos, endpos, count);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "error starting play.");
    return 2;
  }
}

/*** Stops playing the current pattern.
 *
 * @function stop
 *
 */
static int lfun_stop(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);

  int result = blink1_playloop(bd->device, PATTERNPLAY_STOP, 0, 0, 0);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "Error stopping play.");
    return 2;
  }
}

// NOTE: playcount counts down to 0 -- unless it started at 0
// TODO: document the return value(s).
// TODO: maybe return as a table instead ?
/*** Retrieves information on current play state.
 *
 * @function readplay
 *
*/
static int lfun_readplay(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);

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
    lua_pushstring(L, "could not retrieve playstate");
    return 2;
  }
}

/*** Write pattern description for specified position.
 *
 * @function writepatternline
 *
 */
static int lfun_writePatternLine(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);

  int millis = luaL_checkinteger(L, 2);
  int r = luaL_checkinteger(L, 3);
  int g = luaL_checkinteger(L, 4);
  int b = luaL_checkinteger(L, 5);
  int pos = luaL_checkinteger(L, 6);

  luaL_argcheck(L, ( -1 < millis && millis < 32768), 2, "milliseconds must be in range [0, 32768]");
  luaL_argcheck(L, ( -1 < r && r < 256), 3, BADRED_MSG);
  luaL_argcheck(L, ( -1 < g && g < 256), 4, BADGREEN_MSG);
  luaL_argcheck(L, ( -1 < b && b < 256), 5, BADBLUE_MSG);
  // TODO: make upper bound dependent on whether it's a mk2, etc
  luaL_argcheck(L, ( -1 < pos && pos < 33 ), 6, "position must be in range [0, 32]");


  int result = blink1_writePatternLine(bd->device, millis, r, g, b, pos);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
    // TODO: add pos to error message
    lua_pushstring(L, "Could not write pattern line");
    return 2;
  }
}

// TODO: return value as table?
/*** Read pattern description at specified position.
 *
 * @function readpatternline
 *
 */
static int lfun_readPatternLine(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  
  int pos = luaL_checkinteger(L, 2);
  // TODO: make upper bound dependent on whether it's a mk2, etc
  luaL_argcheck(L, ( -1 < pos && pos < 33), 2, "position must be in range [0, 32]");

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
    // TODO: add pos to error message
    lua_pushstring(L, "Could not read pattern line");
    return 2;
  }
}

/*** Saves pattern from RAM into flash.
 *
 * @function savepattern
 *
 */
static int lfun_savePattern(lua_State *L) {
  blinker *bd = luaL_checkudata(L, 1, BLINK_TYPENAME);
  
  int result = blink1_savePattern(bd->device);

  if (result != BLINK1_ERR) {
    lua_pushboolean(L, 1);
    return 1;
  } else {
    lua_pushnil(L);
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
 * List of methods to install in the Blink metatable.
 *
 */
static const luaL_Reg lblink_methods[] = {
  {"isMk2", lfun_isMk2},
  {"serial", lfun_serialNumber},
  {"type", lfun_type},
  {"version", lfun_firmwareVersion},

  {"black", lfun_setBlack},
  {"blue", lfun_setBlue},
  {"brighten", lfun_brighten},
  {"dim", lfun_dim},
  {"cyan", lfun_setCyan},
  {"fade", lfun_fadeToRGB}, 
  {"get", lfun_readRGB}, 
  {"green", lfun_setGreen},
  {"magenta", lfun_setMagenta},
  {"off", lfun_setOff},
  {"on", lfun_setOn},
  {"orange", lfun_setOrange},
  {"red", lfun_setRed},
  {"set", lfun_setRGB},
  {"white", lfun_setWhite},
  {"yellow", lfun_setYellow},
  
  {"play", lfun_play},
  {"readplay", lfun_readplay},
  {"stop", lfun_stop},

  {"readpatternline", lfun_readPatternLine},
  {"savepattern", lfun_savePattern},
  {"writepatternline", lfun_writePatternLine},

  {"__gc", lfun_close},
  {"__tostring", lfun_tostring},
  {"close", lfun_close},
  {NULL, NULL}
};

/*
 *
 * List of functions to install in the library table.
 *
 */
static const luaL_Reg lblink_functions[] = {
  {"enumerate", lfun_enumerate},
  {"gamma", lfun_yesDegamma},
  {"list", lfun_list}, 
  {"open", lfun_open},
  {"noGamma", lfun_noDegamma},
  {"pid", lfun_pid},
  {"sleep", lfun_sleep},
  {"vid", lfun_vid},
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
LUABLINK_API int luaopen_blink(lua_State *L) {
  // Blink metatable
  luaL_newmetatable(L, BLINK_TYPENAME);

  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");

  luaL_setfuncs(L, lblink_methods, 0);

  // library table
  luaL_newlib(L, lblink_functions);

  lua_pushnumber(L, blink1_vid());
  lua_setfield(L, -2, VID_KEY);

  lua_pushnumber(L, blink1_pid());
  lua_setfield(L, -2, PID_KEY);
  
  lua_pushstring(L, LUABLINK_VERSION);
  lua_setfield(L, -2, VERSION_KEY);

  return 1;
}
