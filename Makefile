# Makefile for Vibe
ifeq ($(OS),Windows_NT)
  ifeq '$(findstring ;,$(PATH))' ';'
    UNIX_LIKE := FALSE
  else
    UNIX_LIKE := TRUE
  endif
else
  UNIX_LIKE := TRUE
endif

ifeq ($(UNIX_LIKE),FALSE)
  SHELL := powershell.exe
  .SHELLFLAGS := -NoProfile -NoLogo
  MKDIR := @$$null = new-item -itemtype directory -force
  TOUCH := @$$null = new-item -force
  RM := remove-item -force
  CMAKE := cmake
  CMAKE_GENERATOR := Ninja
  #CMAKE_GENERATOR := Unix Makefiles
  define rmdir
    if (Test-Path $1) { remove-item -recurse $1 }
  endef
  CMAKE_DEPS_PRESET ?= windows-vcpkg
  CMAKE_MAIN_PRESET ?= windows-vcpkg
else
  MKDIR := mkdir -p
  TOUCH := touch
  RM := rm -rf
  CMAKE := $(shell (command -v cmake3 || command -v cmake || echo cmake))
  CMAKE_GENERATOR ?= "$(shell (command -v ninja > /dev/null 2>&1 && echo "Ninja") || echo "Unix Makefiles")"
  #CMAKE_GENERATOR := "Unix Makefiles"
  define rmdir
    rm -rf $1
  endef
endif

# set parallel build jobs
# 1 is useful for debugging
PARALLEL := --parallel 14

MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR  := $(dir $(MAKEFILE_PATH))

filter-false = $(strip $(filter-out 0 off OFF false FALSE,$1))
filter-true = $(strip $(filter-out 1 on ON true TRUE,$1))

# See contrib/local.mk.example
-include local.mk

# Build everything: deps, then main (engine libs and tests)
all: deps main

# Default to RelWithDebInfo if not specified
CMAKE_BUILD_TYPE ?= RelWithDebInfo
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)

# Extra CMake flags which extend the default set
CMAKE_EXTRA_FLAGS ?=
VIBE_PRG := $(MAKEFILE_DIR)/build/bin/vibe

# CMAKE_INSTALL_PREFIX
#   - May be passed directly or as part of CMAKE_EXTRA_FLAGS.
#   - `checkprefix` target checks that it matches the CMake-cached value. #9615
ifneq (,$(CMAKE_INSTALL_PREFIX)$(CMAKE_EXTRA_FLAGS))
CMAKE_INSTALL_PREFIX := $(shell echo $(CMAKE_EXTRA_FLAGS) | 2>/dev/null \
    grep -o 'CMAKE_INSTALL_PREFIX=[^ ]\+' | cut -d '=' -f2)
endif
ifneq (,$(CMAKE_INSTALL_PREFIX))
override CMAKE_EXTRA_FLAGS += -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)

checkprefix:
	@if [ -f build/.ran-cmake ]; then \
	  cached_prefix=$(shell $(CMAKE) -L -N build | 2>/dev/null grep 'CMAKE_INSTALL_PREFIX' | cut -d '=' -f2); \
	  if ! [ "$(CMAKE_INSTALL_PREFIX)" = "$$cached_prefix" ]; then \
	    printf "Re-running CMake: CMAKE_INSTALL_PREFIX '$(CMAKE_INSTALL_PREFIX)' does not match cached value '%s'.\n" "$$cached_prefix"; \
	    $(RM) build/.ran-cmake; \
	  fi \
	fi
else
checkprefix: ;
endif

DEPS_BUILD_DIR ?= ".deps"
BUILD_DIR ?= "build"

ifneq (1,$(words [$(DEPS_BUILD_DIR)]))
  $(error DEPS_BUILD_DIR must not contain whitespace)
endif

DEPS_CMAKE_FLAGS ?=
USE_BUNDLED ?=

ifneq (,$(USE_BUNDLED))
  BUNDLED_CMAKE_FLAG := -DUSE_BUNDLED=$(USE_BUNDLED)
endif

ifneq (,$(findstring functionaltest-lua,$(MAKECMDGOALS)))
  BUNDLED_LUA_CMAKE_FLAG := -DUSE_BUNDLED_LUA=ON
  $(shell [ -x $(DEPS_BUILD_DIR)/usr/bin/lua ] || $(RM) build/.ran-*)
endif

# For use where we want to make sure only a single job is run.  This does issue 
# a warning, but we need to keep SCRIPTS argument.
SINGLE_MAKE = export MAKEFLAGS= ; $(MAKE)

#vibe: build/.ran-cmake deps
main: $(BUILD_DIR)
	$(CMAKE) --build build $(PARALLEL)

$(BUILD_DIR):
ifeq ($(UNIX_LIKE),FALSE)
	$(CMAKE) --preset $(CMAKE_MAIN_PRESET) $(CMAKE_EXTRA_FLAGS);
else
	$(CMAKE) -B $(BUILD_DIR) -G $(CMAKE_GENERATOR) $(CMAKE_FLAGS) $(CMAKE_EXTRA_FLAGS) $(MAKEFILE_DIR);
endif

# engine libs now build in-tree as part of main
vibe libvibe: main

cmake:
	$(TOUCH) CMakeLists.txt
	$(MAKE) build/.ran-cmake

build/.ran-cmake: | deps
ifeq ($(UNIX_LIKE),FALSE)
	$(CMAKE) --preset $(CMAKE_MAIN_PRESET) $(CMAKE_EXTRA_FLAGS)
else
	$(CMAKE) -B build -G $(CMAKE_GENERATOR) $(CMAKE_FLAGS) $(CMAKE_EXTRA_FLAGS) $(MAKEFILE_DIR)
endif
	$(TOUCH) $@

deps: | build/.ran-deps-cmake
ifeq ($(call filter-true,$(USE_BUNDLED)),)
	$(CMAKE) --build $(DEPS_BUILD_DIR) $(PARALLEL)
endif

ifeq ($(call filter-true,$(USE_BUNDLED)),)
$(DEPS_BUILD_DIR):
	$(MKDIR) $@

build/.ran-deps-cmake:: $(DEPS_BUILD_DIR)
ifeq ($(UNIX_LIKE),FALSE)
	$(CMAKE) --preset $(CMAKE_DEPS_PRESET) -S $(MAKEFILE_DIR)/deps $(BUNDLED_CMAKE_FLAG) $(BUNDLED_LUA_CMAKE_FLAG) $(DEPS_CMAKE_FLAGS)
else
	$(CMAKE) -S $(MAKEFILE_DIR)/deps -B $(DEPS_BUILD_DIR) -G $(CMAKE_GENERATOR) $(BUNDLED_CMAKE_FLAG) $(BUNDLED_LUA_CMAKE_FLAG) $(DEPS_CMAKE_FLAGS)
	#$(CMAKE) -S $(MAKEFILE_DIR)/deps -B $(DEPS_BUILD_DIR) -G $(CMAKE_GENERATOR) $(BUNDLED_CMAKE_FLAG) $(BUNDLED_LUA_CMAKE_FLAG) $(DEPS_CMAKE_FLAGS)
endif
endif
build/.ran-deps-cmake::
	$(MKDIR) build
	$(TOUCH) "$@"


test: main
	ctest --test-dir build --output-on-failure

clean:
	$(call rmdir, build)

# also deletes .deps/ (the third-party superbuild, a long rebuild)
distclean: clean
	$(call rmdir, $(DEPS_BUILD_DIR))

INSTALL_PREFIX ?= /usr/local

install: checkprefix main
	$(CMAKE) --install build --prefix $(INSTALL_PREFIX)

.PHONY: test clean distclean vibe libvibe cmake deps install checkprefix
