
# Blink

ThingM's [blink(1)](https://blink1.thingm.com/) is a programmable status indiator that plugs into a USB port. This library allows you to control a blink(1) via Lua code.


## Usage

If you just want a pretty light, it's as simple as:

    blink = require 'blink'
    d = blink.open()
    d:set(255, 123, 147)



This project uses semantic versioning. See <a href="http://semver.org">semver.org</a> for more information.

## Requirements and Installation

The library was developed and tested using [Lua](http://lua.org) 5.2.3. However it should run on other versions of Lua with little or no modification.

It does depend on the Blink1 library (available from ...). Currently the makefile assumes Blink1 is already built and available. Future versions of the build process will handle downloading, building and installing this library as needed.

*TODO*: installation instructions and a rockspec for LuaRocks.


## Documentation and Contact Information

More documentation be found in the `doc` subdirectory. The best way to contact me regarding this library is to post an issue at the [github repository](https://github.com/profburke/luablink/issues).


## License

Blink is free software distributed under the terms of the MIT license. It may be used for any purpose, including commercial purposes, at absolutely no cost without having to ask us. The only requirement is that if you do use blink, then you should give us credit by including the appropriate copyright notice somewhere in your product or its documentation. For details, see LICENSE.txt.

