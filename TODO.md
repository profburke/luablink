### TODO Items

- improved build process and/or a rockspec (top priority)
- implement an "all" ID to turn them all off, set them all to red, etc
- implement an "all" LED # to affect both LEDs on a device
- support setting only 1 of the LEDs; however this isn't supported by the blink library so we 
     need to think about how best to do this (possibly use fade w/a very short time)
- {"serverdown", lfun_serverdown},
- add "Release History" to README (see https://github.com/rgieseke/textui)


### Unimplemented

- clear pattern
- servertickle
- hsbtorgb (and vice versa?)
- parsepattern (?)
- set entire pattern (all 32 slots)  as string
- get entire pattern (as string)


### Misc

- isolate from changes in blink1-lib; at the very least, specify a commit hash that this code is known to work with
- improve docs: content and formatting
- add some more examples (link to Morse and Color Swatches)
- link to docs from README
- maybe a GH Pages site to display the docs (and a link from README)
- improve the makefile
- add additional functionality in Lua, such as clearpattern, chase, glitter, etc
- rethink some of the function names
- optional arg to stop command that also turns blink off
