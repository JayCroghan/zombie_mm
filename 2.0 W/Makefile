#(C)2004-2005 SourceMM Development Team
# Makefile written by David "BAILOPAN" Anderson

#HL2SDK = /cygdrive/e/coding/c/hl2sdk/cstrike
#SMM_ROOT = /cygdrive/e/coding/c/sourcemm
#SRCDS = /cygdrive/e/coding/c/hlds_l

HL2SDK = /cygdrive/c/Code/c/hl2sdk/cstrike
SMM_ROOT = /cygdrive/c/Code/c/sourcemm
SRCDS = /cygdrive/c/Code/c/hlds_l

### EDIT BELOW FOR OTHER PROJECTS ###
OPT_FLAGS = -O3 -fno-rtti -funroll-loops
#OPT_FLAGS = -O0 -g -ggdb3 
#DEBUG_FLAGS = -O0 -g -ggdb3 -pipe
#DEBUG_FLAGS = -O0 -g -ggdb3 
DEBUG_FLAGS = -g -ggdb3 
CPP = /opt/crosstool/gcc-3.4.1-glibc-2.3.2/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc
BINARY = zombie_mm_i486.so

#OBJECTS = ZombiePlugin.cpp MRecipients.cpp bitbuf.cpp cvars.cpp CSigMngr.cpp SDK.cpp mathlib.cpp VFunc.cpp myutils.cpp KeyValues.cpp utlbuffer.cpp ZM_Util.cpp
OBJECTS = ZombiePlugin.cpp MRecipients.cpp cvars.cpp CSigMngr.cpp SDK.cpp mathlib.cpp VFunc.cpp myutils.cpp ZM_Util.cpp KeyValues.cpp utlbuffer.cpp bitbuf.cpp Events.cpp

LINK = vstdlib_i486.so tier0_i486.so

HL2PUB = $(HL2SDK)/public

INCLUDE = -I. -I$(HL2PUB) -I$(HL2PUB)/dlls -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 \
-I$(HL2PUB)/vstdlib -I$(HL2SDK)/tier1  -I$(HL2SDK)/game_shared -I$(HL2SDK)/dlls \
-I$(SMM_ROOT) -I$(SMM_ROOT)/sourcehook -I$(SMM_ROOT)/sourcemm

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS = $(DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS = $(OPT_FLAGS)
endif


ifeq "$(ICEPICK)" "true"
	BIN_DIR = Icepick
endif

CFLAGS = $(OPT_FLAGS)

#CFLAGS += -v -fpermissive -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -fPIC -Wno-deprecated -static-libgcc -m3dnow -msse
CFLAGS += -v -fpermissive -D_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -fPIC -Wno-deprecated -static-libgcc -m3dnow -msse

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)
	ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so
	ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so
	ln -sf $(HL2SDK)/tier1/convar.cpp convar.cpp
	$(MAKE) sourcemm
	rm -rf $(BINARY)
	ln -sf $(BIN_DIR)/$(BINARY) $(BINARY)

sourcemm: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(CFLAGS) $(OBJ_LINUX) $(LINK) -shared -lm -o$(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) all DEBUG=true
	
icepick:
	$(MAKE) all ICEPICK=true

default: all

clean:
	rm -rf Release/*.o
	rm -rf Release/$(BINARY)
	rm -rf Debug/*.o
	rm -rf Debug/$(BINARY)
