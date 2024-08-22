SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
ifeq ($(origin .RECIPEPREFIX), undefined)
  $(error This Make does not support .RECIPEPREFIX. Please use GNU Make 4.0 or later)
endif
.RECIPEPREFIX = >

# One of these days, I'll fix this top-level makefile up so that you can build and install the code
# and make the docs from here, but for now ...

docs:
> ldoc -c ./config.ld src/blink.c
> cp -f ./ldoc.css doc
.PHONY: docs

