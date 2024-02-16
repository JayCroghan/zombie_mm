# (C)2004-2010 Metamod:Source Development Team
# Makefile written by David "BAILOPAN" Anderson

###########################################
### EDIT THESE PATHS FOR YOUR OWN SETUP ###
###########################################

HL2SDK_OB_VALVE = /cygdrive/c/code/c/hl2sdk-ob-0f3afddb27c9
MMSOURCE18 = /cygdrive/c/code/c/mmsource-1-8-9c2b1e877e16
SOURCEMOD = /cygdrive/c/code/c/sourcemod-1-3-4d6afc42522c

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = zombie_mm
OBJECTS = ZombiePlugin.cpp MRecipients.cpp cvars.cpp CSigManager.cpp SDK.cpp VFunc.cpp myutils.cpp ZM_Util.cpp Events.cpp zm_linuxutils.cpp cp-demangle.cpp cp-demint.cpp cplus-dem.cpp safe-ctype.cpp xmalloc.cpp xstrdup.cpp zm_memory.cpp sm_trie.cpp css_offsets.cpp ZMClass.cpp

#mathlib.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

OPT_FLAGS = -O3 -funroll-loops -s -pipe
DEBUG_FLAGS = -g -ggdb3 -D_DEBUG


CFLAGS = -v -fpermissive -D_LINUX -DPLATFORM_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -fPIC -Wno-deprecated -m3dnow -msse -D_BETA=1 -DORANGEBOX_BUILD=1

CPP = /opt/crosstool/gcc-3.4.1-glibc-2.3.2/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc



##########################
### SDK CONFIGURATIONS ###
##########################

CFLAGS += -DSOURCE_ENGINE=4

HL2PUB = $(HL2SDK_OB_VALVE)/public

METAMOD = $(MMSOURCE18)/core

LIB_EXT = so
HL2LIB = $(HL2SDK_OB_VALVE)/lib/linux


LIB_SUFFIX = _i486.$(LIB_EXT)

LINK += $(HL2LIB)/tier1_i486.a libtier0.so libvstdlib.so $(HL2LIB)/mathlib_i486.a -static-libgcc -shared 
#LINK += $(HL2LIB)/tier1_i486.a $(LIB_PREFIX)vstdlib$(LIB_SUFFIX) $(LIB_PREFIX)tier0$(LIB_SUFFIX) -static-libgcc
	
#INCLUDE += -I. -I.. -I$(HL2PUB) -I$(SOURCEMOD)/public -I$(SOURCEMOD)/public/extensions -I$(SOURCEMOD)/public/sourcepawn -I$(HL2SDK_OB_VALVE)/game/shared -I$(HL2PUB)/game/server -I$(HL2SDK_OB_VALVE)/game/server -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 -I$(HL2PUB)/vstdlib -I$(HL2SDK_OB_VALVE)/tier1 -I$(MMSOURCE18) -I$(MMSOURCE18)/core -I$(MMSOURCE18)/core/sourcehook -I$(HL2PUB)/mathlib -I$(SOURCEMOD)/extensions/sdktools/sdk

$(VC_IncludePath);$(WindowsSDK_IncludePath);d:\Old Machine\Code\c\mmsource-1-8-9c2b1e877e16;

d:\Old Machine\Code\c\sourcemod-1-3-4d6afc42522c\public;
d:\Old Machine\Code\c\sourcemod-1-3-4d6afc42522c\public\extensions;
d:\Old Machine\Code\c\sourcemod-1-3-4d6afc42522c\public\sourcepawn\;
d:\Old Machine\Code\c\sourcemod-1-3-4d6afc42522c\core\;
d:\Old Machine\Code\c\sourcemod-1-3-4d6afc42522c\core\extensions\sdktools\sdk\;
d:\Old Machine\Code\c\mmsource-1-8-9c2b1e877e16\;
d:\Old Machine\Code\c\mmsource-1-8-9c2b1e877e16\core\sourcehook;
d:\Old Machine\Code\c\mmsource-1-8-9c2b1e877e16\core\;
D:\Old Machine\Code\C\hl2sdk-ob-0f3afddb27c9\public\tier1;
D:\Old Machine\Code\C\hl2sdk-ob-0f3afddb27c9\game\shared\;
D:\Old Machine\Code\C\hl2sdk-ob-0f3afddb27c9\public\game\server;
D:\Old Machine\Code\C\hl2sdk-ob-0f3afddb27c9\game\server\;
D:\Old Machine\Code\C\hl2sdk-ob-0f3afddb27c9\public\;
D:\Old Machine\Code\C\hl2sdk-ob-0f3afddb27c9\public\engine\;

INCLUDE += -I. -I.. -I$(SOURCEMOD)/public -I$(SOURCEMOD)/public/extensions -I$(SOURCEMOD)/public/sourcepawn -I$(HL2SDK_OB_VALVE)/game/shared -I$(HL2SDK_OB_VALVE)/public/game/server -I$(HL2SDK_OB_VALVE)/game/server -I$(HL2SDK_OB_VALVE)/public -I$(HL2SDK_OB_VALVE)/public/engine -I$(HL2SDK_OB_VALVE)/public/tier0 -I$(HL2SDK_OB_VALVE)/public/tier1 -I$(HL2SDK_OB_VALVE)/public/vstdlib -I$(HL2SDK_OB_VALVE)/tier1 -I$(MMSOURCE18) -I$(MMSOURCE18)/core -I$(MMSOURCE18)/core/sourcehook -I$(HL2SDK_OB_VALVE)/public/mathlib -I$(SOURCEMOD)/extensions/sdktools/sdk
INCLUDE += -I..\sdk -I$(SOURCEMOD)/core

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

BINARY = $(PROJECT)$(LIB_SUFFIX)


ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS += $(DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS += $(OPT_FLAGS)
endif


GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)

LIB_EXT = so

OBJ_BIN := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all: check
	mkdir -p $(BIN_DIR)
	ln -sf $(HL2LIB)/libvstdlib.so libvstdlib.so
	ln -sf $(HL2LIB)/libtier0.so libtier0.so
	$(MAKE) zombiemod_mm
	
check:

zombiemod_mm: check $(OBJ_BIN)
	$(CPP) $(INCLUDE) -m32 $(OBJ_BIN) $(LINK) -ldl -lm -o$(BIN_DIR)/$(BINARY)


#ln -s `/opt/crosstool/gcc-3.4.1-glibc-2.3.2/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc -print-file-name=libstdc++.a`
#ln -s `/opt/crosstool/gcc-3.4.1-glibc-2.3.2/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc -print-file-name=libstdc++.a`

default: all

clean: check
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/$(BINARY)
