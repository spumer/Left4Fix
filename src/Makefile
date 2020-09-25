# (C)2004-2010 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson and modified by spumer

###########################################
### EDIT THESE PATHS FOR YOUR OWN SETUP ###
###########################################

SDK_PLATFORM ?= ../../../_SDK_PLATFORM

SMSDK = $(SDK_PLATFORM)/sourcemod
SRCDS_BASE ?= $(SDK_PLATFORM)/srcds
HL2SDK_L4D2 ?= $(SDK_PLATFORM)/hl2sdk-l4d2
MMSOURCE ?= $(SDK_PLATFORM)/mmsource

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = left4fix

#Uncomment for Metamod: Source enabled extension
#USEMETA = true

OBJECTS = asm/asm.c sdk/smsdk_ext.cpp extension.cpp natives.cpp util.cpp routine.cpp \
		detours/detour.cpp detours/detour_function.cpp detours/detour_call.cpp \
		detours/on_revived_by_defib.cpp detours/on_recompute_versus_completion.cpp \
		detours/on_get_completion_by_character.cpp codepatch/patchmanager.cpp codepatch/warp_max_players.cpp \
		detours/on_warp_ghost_call_get_player.cpp codepatch/tiebreak.cpp

ifeq "$(TEAM_SIZE)" "8"
	OBJECTS += codepatch/score_code_lite.cpp
	CFLAGS += -DSCORE_LITE
else
	OBJECTS += codepatch/score_code_linux.cpp
endif

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -Ofast -flto -funroll-loops -pipe -fno-strict-aliasing -fschedule-insns -fsched-pressure
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden -std=c++11
CPP = gcc-4.8

##########################
### SDK CONFIGURATIONS ###
##########################

override ENGSET = false

HL2PUB = $(HL2SDK_L4D2)/public
HL2LIB = $(HL2SDK_L4D2)/lib/linux
CFLAGS += -DSOURCE_ENGINE=6
METAMOD = $(MMSOURCE)/core
INCLUDE += -I$(HL2SDK_L4D2)/public/game/server -I$(HL2SDK_L4D2)/common -I$(HL2SDK_L4D2)/game/shared
SRCDS = $(SRCDS_BASE)/left4dead2

LINK += $(HL2LIB)/tier1_i486.a $(HL2LIB)/mathlib_i486.a libvstdlib_srv.so libtier0_srv.so

INCLUDE += -I. -I.. -Isdk -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/mathlib -I$(HL2PUB)/tier0 \
        -I$(HL2PUB)/tier1 -I$(METAMOD) -I$(METAMOD)/sourcehook -I$(SMSDK)/public -I$(SMSDK)/public/extensions \
        -I$(SMSDK)/public/sourcepawn

CFLAGS += -DSE_EPISODEONE=1 -DSE_DARKMESSIAH=2 -DSE_ORANGEBOX=3 -DSE_ORANGEBOXVALVE=4 -DSE_LEFT4DEAD=5 -DSE_LEFT4DEAD2=6

LINK += -m32 -ldl -lm

CFLAGS += -D_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
        -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror -Wno-switch \
        -Wno-unused -mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H -m32 -DGNUC -D__thiscall= -DTEAM_SIZE=$(TEAM_SIZE)

CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wno-overloaded-virtual

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS += $(C_DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS += $(C_OPT_FLAGS)
endif

ifeq "$(OS)" "Darwin"
	LIB_EXT = dylib
	CFLAGS += -isysroot /Developer/SDKs/MacOSX10.5.sdk
	LINK += -dynamiclib -lstdc++ -mmacosx-version-min=10.5
else
	LIB_EXT = so
	CFLAGS += -D_LINUX
	LINK += -static-libgcc -shared
endif

BINARY = $(PROJECT).ext.$(LIB_EXT)

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)

# apply flags for GCC 4, 5 and 6 versions
ifneq (,$(filter $(GCC_VERSION),4 5 6))
	CFLAGS += $(C_GCC4_FLAGS)
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

OBJ_BIN := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

ifndef TEAM_SIZE
	$(error Please set the TEAM_SIZE. Exmpl: make TEAM_SIZE=10)
endif

all:
	mkdir -p $(BIN_DIR)/sdk
	mkdir -p $(BIN_DIR)/detours
	mkdir -p $(BIN_DIR)/codepatch
	mkdir -p $(BIN_DIR)/l4d2sdk
	ln -sf $(SRCDS)/bin/libvstdlib_srv.so libvstdlib_srv.so;
	ln -sf $(SRCDS)/bin/libtier0_srv.so libtier0_srv.so;
	$(MAKE) -f Makefile extension TEAM_SIZE=$(TEAM_SIZE)

extension: $(OBJ_BIN)
	$(CPP) $(INCLUDE) $(OBJ_BIN) $(LINK) -o $(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean:
	rm -rf Debug/ Release/
