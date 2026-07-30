# ============================================================================
#  PS4 IPTV Player - OpenOrbis PS4 Toolchain build file
# ============================================================================
#  Requires the OO_PS4_TOOLCHAIN environment variable to point at a checkout
#  of the OpenOrbis PS4 Toolchain (https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain).
# ============================================================================

# Package metadata.
TITLE       := PS4 IPTV Player
VERSION     := 1.00
TITLE_ID    := IPTV00001
CONTENT_ID  := IV0000-IPTV00001_00-PS4IPTVPLAYER000

# Libraries linked into the ELF.
LIBS        := -lc -lkernel -lc++ \
               -lSceUserService -lSceSystemService -lSceVideoOut -lSceAudioOut \
               -lScePad -lSceSysmodule -lSceFreeType \
               -lSceNet -lSceSsl -lSceHttp \
               -lSceAvPlayer \
               -lSceCommonDialog -lSceImeDialog \
               -lSDL2 -lSDL2_image

# Additional compile flags.
EXTRAFLAGS  := -Wall -Wno-unused-function -O2 -Iinclude -D_BSD_SOURCE

# ---------------------------------------------------------------------------
# Optional software (FFmpeg) decoder backend for AV1 / VP9 / VVC.
#
#   make USE_FFMPEG=1 FFMPEG_DIR=/path/to/ffmpeg-ps4-prefix
#
# FFMPEG_DIR must point at a prefix containing include/ and lib/ with FFmpeg
# built for the OpenOrbis PS4 target (see tools/build_ffmpeg.sh). When unset
# the default build uses only the PS4 hardware decoder (libSceAvPlayer).
# ---------------------------------------------------------------------------
USE_FFMPEG  ?= 0
ifeq ($(USE_FFMPEG),1)
EXTRAFLAGS  += -DUSE_FFMPEG -I$(FFMPEG_DIR)/include
LIBS        += -L$(FFMPEG_DIR)/lib -lavformat -lavcodec -lswscale -lswresample -lavutil
endif

# Asset and module directories.
ASSETS      := $(shell find assets -type f)
LIBMODULES  := $(wildcard sce_module/*)

# You likely won't need to touch anything below this point.

# Root vars
TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
PROJDIR     := source
INTDIR      := build

# Define objects to build (recursively scan the source directory)
CFILES      := $(shell find $(PROJDIR) -name '*.c')
CPPFILES    := $(shell find $(PROJDIR) -name '*.cpp')
OBJS        := $(patsubst $(PROJDIR)/%.c, $(INTDIR)/%.o, $(CFILES)) \
               $(patsubst $(PROJDIR)/%.cpp, $(INTDIR)/%.o, $(CPPFILES))

# Define final C/C++ flags
CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -std=c++17
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

# Check for linux vs macOS and account for clang/ld path
UNAME_S     := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
		CC      := clang
		CCX     := clang++
		LD      := ld.lld
		CDIR    := linux
endif
ifeq ($(UNAME_S),Darwin)
		CC      := /usr/local/opt/llvm/bin/clang
		CCX     := /usr/local/opt/llvm/bin/clang++
		LD      := /usr/local/opt/llvm/bin/ld.lld
		CDIR    := macos
endif

all: $(CONTENT_ID).pkg

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core pkg_build $< .

pkg.gp4: eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png $(LIBMODULES) $(ASSETS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

sce_sys/param.sfo: Makefile
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

eboot.bin: $(OBJS)
	$(LD) $(OBJS) -o $(INTDIR)/iptv.elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(INTDIR)/iptv.elf -out=$(INTDIR)/iptv.oelf --eboot "eboot.bin" --paid 0x3800000000000011

$(INTDIR)/%.o: $(PROJDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/%.o: $(PROJDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CCX) $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(INTDIR) $(CONTENT_ID).pkg pkg.gp4 eboot.bin sce_sys/param.sfo
