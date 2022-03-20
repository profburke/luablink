
# Blink

ThingM's [blink(1)](https://blink1.thingm.com/) is a programmable status indicator that plugs into a USB port. This library allows you to control a blink(1) via Lua code.


## Usage

If you just want a pretty light, it's as simple as:

```lua
    blink = require 'blink'
    d = blink.open()
    d:set(255, 123, 147)
```

A slightly more extensive example to toggle the blink between red and blue:

```lua
    local blink = require 'blink'

    local function doit(d, reps, time)
      time = time or 300
      for i = 1,reps do
        d:red(); blink.sleep(time); d:blue(); blink.sleep(time);
       end
       d:off()
    end

    local d = blink.open()

    if d then
      doit(d, 4)
    end
```
              
## Requirements and Installation

**As of March 2022**, it has been updated to work with Lua 5.4.2.

The library was developed and tested using [Lua](http://lua.org) 5.2.3. However it should run on other versions of Lua with little or no modification. 

It does depend on the Blink1 [command line tool](https://github.com/todbot/blink1/blob/master/docs/blink1-tool.md). Currently the makefile assumes this is already built and, specifically, that the include and dynamic library files are available in `/usr/local/include` and `/usr/local/lib` respectively. Future versions of the build process will handle downloading, building and installing this library as needed.

## Documentation

This project uses semantic versioning. See <a href="http://semver.org">semver.org</a> for more information. See the [Changelog](https://github.com/profburke/luablink/blob/master/Changelog.md) for a summary of the project's evolution. Check the [TODO](https://github.com/profburke/luablink/blob/master/TODO.md) for a list or possible/planned improvements.

API documentation be found in the `doc` subdirectory, the project uses [ldoc](https://stevedonovan.github.io/ldoc/) to generate the documentation.

## Contributions and Contact Information

**Help and contributions are encouraged and greatly appreciated!** 

Code changes, documentation improvement, more examples, ..., a cool logo&mdash;there are a wide range of ways you can contribute. The best way to contact me regarding this library is to post an issue at the [github repository](https://github.com/profburke/luablink/issues).

## License

Blink is free software distributed under the terms of the MIT license. It may be used for any purpose, including commercial purposes, at absolutely no cost without having to ask us. The only requirement is that if you do use blink, then you should give us credit by including the appropriate copyright notice somewhere in your product or its documentation. For details, see [LICENSE](https://github.com/profburke/luablink/blob/master/LICENSE).

