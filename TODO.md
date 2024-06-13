### TODO Items

- improved build process and/or a rockspec (top priority)
- implement an "all" ID to turn them all off, set them all to red, etc
- implement an "all" LED # to affect both LEDs on a device
- support setting only 1 of the LEDs; however this isn't supported by the blink library so we 
     need to think about how best to do this (possibly use fade w/a very short time)
- {"serverdown", lfun_serverdown},
- add "Release History" to README (see https://github.com/rgieseke/textui)

### Completed Items
- Methods should give errors when called on closed devices.

- what about new functionality?
- look at blink1-tool
- look at python and other language libs

- make decisions about #define vs static const
- finish moving all (most) string literals

### Unimplemented

- clear pattern
- servertickle
- hsbtorgb (and vice versa?)
- parsepattern (?)
- set entire pattern (all 32 slots)  as string
- get entire pattern (as string)

