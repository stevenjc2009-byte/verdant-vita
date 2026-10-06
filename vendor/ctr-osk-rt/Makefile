#---------------------------------------------------------------------------------
# Builds lib/libctrosk.a. Install it into a devkitPro portlibs tree with
# `make install`, or just point your app's LIBDIRS at this directory.
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET   := ctrosk
BUILD    := build
SOURCES  := source
INCLUDES := include source

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -Wextra -O2 -mword-relocations \
          -ffunction-sections -fdata-sections \
          $(ARCH) $(INCLUDE) -D__3DS__

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT  := $(CURDIR)/lib/lib$(TARGET).a
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES  := $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  -I$(CTRULIB)/include

.PHONY: all clean install example

all: lib $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

lib:
	@mkdir -p lib

$(BUILD):
	@mkdir -p $@

example: all
	@$(MAKE) --no-print-directory -C example

clean:
	@echo clean ...
	@rm -rf $(BUILD) lib
	@$(MAKE) --no-print-directory -C example clean

# Drop the header and archive into the devkitPro portlibs tree so other
# projects can just add -lctrosk.
install: all
	@mkdir -p $(DESTDIR)$(PORTLIBS)/lib $(DESTDIR)$(PORTLIBS)/include
	@cp lib/lib$(TARGET).a $(DESTDIR)$(PORTLIBS)/lib/
	@cp include/ctrosk.h   $(DESTDIR)$(PORTLIBS)/include/
	@echo installed to $(DESTDIR)$(PORTLIBS)

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT): $(OFILES)
	@rm -f $@
	@$(AR) rcs $@ $(OFILES)
	@echo built ... $(notdir $@)

-include $(DEPENDS)

endif
