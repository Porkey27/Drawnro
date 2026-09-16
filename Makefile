ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET      :=  drawnro
BUILD       :=  build
SOURCES     :=  source
INCLUDES    :=  source

APP_TITLE   :=  DrawNRO
APP_AUTHOR  :=  sporf
APP_VERSION :=  1.0.0

ARCH    :=  -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE

PORTLIBS := $(PORTLIBS_PATH)/switch
LIBNX    := $(DEVKITPRO)/libnx

CFLAGS  :=  -g -Wall -O2 -ffunction-sections \
            $(ARCH) $(BUILD_CFLAGS) \
            `$(PORTLIBS)/bin/sdl2-config --cflags` \
            -D__SWITCH__ -DAPP_VERSION=\"$(APP_VERSION)\"

CFLAGS  +=  $(INCLUDE)

CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17

ASFLAGS :=  -g $(ARCH)
LDFLAGS  =  -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS    :=  -lSDL2_ttf -lSDL2_image -lSDL2 -lfreetype -lharfbuzz -lpng -ljpeg -lbz2 -lz -lnx -lstdc++ -lm

LIBDIRS := $(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   :=  $(CURDIR)/$(TARGET)
export TOPDIR   :=  $(CURDIR)

export VPATH    :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export DEPSDIR  :=  $(CURDIR)/$(BUILD)

CFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export LD   :=  $(CC)

export OFILES_SRC   :=  $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES   :=  $(OFILES_SRC)

export INCLUDE  :=  $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                     $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                     -I$(CURDIR)/$(BUILD)

export LIBPATHS :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

else

DEPENDS :=  $(OFILES:.o=.d)

all  :   $(OUTPUT).nro

$(OUTPUT).nro : $(OUTPUT).elf $(OUTPUT).nacp

$(OUTPUT).elf  :   $(OFILES)

-include $(DEPENDS)

endif
