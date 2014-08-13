#ifndef LUABLINK_H
#define LUABLINK_H
/*=========================================================================*\
* LuaBlink
* Lua binding to the Blink(1) library.
* Matthew M. Burke <matthew@bluedino.net>
* 2014-08-09
\*=========================================================================*/
#include "lua.h"

extern const char * LUABLINK_VERSION;
#define LUABLINK_COPYRIGHT "Copyright (C) 2014 Matthew M. Burke"


/*-------------------------------------------------------------------------*\
* This macro prefixes all exported API functions
\*-------------------------------------------------------------------------*/
#ifndef LUABLINK_API
#define LUABLINK_API extern
#endif


/*-------------------------------------------------------------------------*\
* Initializes the library.
\*-------------------------------------------------------------------------*/
LUABLINK_API int luaopen_blink(lua_State *L);


#endif /* LUABLINK_H */


