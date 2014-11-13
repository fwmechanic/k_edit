#
# Copyright 2011-2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
#
# When building on Windows, uses MinGW tools from http://nuwen.net/mingw.html
#
# gmake automatic variables: http://www.gnu.org/savannah-checkouts/gnu/make/manual/html_node/Automatic-Variables.html
#
# $@  The file name of the target of the rule.
# $<  The name of the first prerequisite.
# $?  The names of all the prerequisites that are newer than the target, with spaces between them.
# $^  The names of all the prerequisites, with spaces between them.
# $+  This is like '$^', but prerequisites listed more than once are duplicated in the order they were listed in the makefile.
# $|  The names of all the order-only prerequisites, with spaces between them.
# $*  The stem with which an implicit rule matches
#

.PHONY: zap clean cleanliblua

# APP_IN_DLL = 1

# 'ifdef ComSpec' -> 'if building on Windows'
ifdef ComSpec
PLAT = mingw
export PLAT

# SHELL=cmd is a nasty hack since it is (a) nonportable and (b) has to be
# done in every makefile (in the event of recursive make'ing), rather than
# being inherited from the "parent" make process.  Therefore I had to also
# add it to Lua-5.1\src\Makefile
#
# Why is it necessary? Because msysgit (for some install variants) puts a
# directory "C:\Program Files (x86)\Git\bin" which contains an instance
# of sh.exe, in PATH.  In the past I've renamed this file to sh_.exe and
# lived to tell the tale, but my use case for git (perhaps uniquely) did
# not include `git clone`, which I've lately started to use.  `git clone`
# fails if it cannot find (its) sh.exe.  And I've found it useful to have
# this dir in PATH since these versions of the "unix utils" tend to be
# quite up-to-date and "come for free" since I religiously update msysgit
# on each release.
#
SHELL=cmd
export SHELL

CMDTBL_ARG=win32
# dflt $(RM) for MinGW is rm -f
RM= del /F /Q
export RM
MV = move
export MV
EXE_EXT := .exe
DLL_EXT := .dll
OS_LIBS := -lpsapi
PLAT_LINK_OPTS=-Wl,--enable-auto-image-base -Wl,--nxcompat
# LS_L_TAIL is from http://ss64.com/nt/dir.html (MS BATch programming and cmdline utils suck SO BAD!)
LS_L := dir
LS_L_TAIL := | FIND "/"
# MinGW _mostly_ works OK using / as dirsep, HOWEVER when specifying a path prefix to argv[0], cmd _requires_ DIRSEP==\ (fails if DIRSEP==/)
DIRSEP := \\

else
PLAT = linux
export PLAT

   # Linux build
   # sudo apt-get install -y libncurses5-dev
   # sudo apt-get install -y libpcre3-dev

CMDTBL_ARG=other
MV = mv
export MV
RM= del /F /Q
export RM
EXE_EXT :=
DLL_EXT := .so
OS_LIBS := -lncurses
PLAT_LINK_OPTS=
LS_L := ls -l
LS_L_TAIL :=
DIRSEP := /

endif

# generally I don't use a debugger, but when crashes occur, obtaining a stack
# trace is VERY helpful.  I use DrMinGW  https://github.com/jrfonseca/drmingw/releases
# note that the version (32- vs. 64-bit) is _app-being-debugged_ specific!  And you can
# simultaneously install BOTH 32- and 64-bit versions of DrMinGW (and MUST if you want
# to be able to debug BOTH 32- and 64-bit versions of this app (K editor)
#
# You MUST rebuild K to get useful info from DrMinGW:
# 1. define DBG_BUILD (uncomment line 'DBG_BUILD := 1' below)
# 2. make clean&&et make -j
# 3. cause k to crash or call abort() due to -ftrapv
# 4. choose [Debug] when Windows crash-dialog appears
# [note] 20140526 I at last discovered that -Wl,--dynamicbase prevents DrMinGW (and perhaps GDB?)
#                 from working. Removal of this option is done automatically in step 1 above.
#
# To run gdb:
#
# 0. (follow steps 1-2 above, building for MinGW)
# a. gdb k    # cannot run 'gdb k -n' to do this in one step
# b. run -n   # -n is option to k to start in new console window, so we can axs GDB when abort is called
#    (follow step 4 above; you will be put into gdb console)
# c. >bt to see stack trace
# d. >q  to exit gdb

# to disable, _comment out_ next line!
# DBG_BUILD := x

ifdef DBG_BUILD
STRIP      :=
C_OPTS_DBG := -g -fno-omit-frame-pointer
else
STRIP      := -s
C_OPTS_DBG :=
endif

LUA_DIR=lua-5.1/src

CC_OUTPUT := # --save-temp
# -fstack-protector    gens link error
# -fsanitize=undefined gens link errors (is supposed to work on Linux x64 w/GCC 4.9.2 only)
# -fstrict-overflow -Wstrict-overflow=2
TRAPV := -ftrapv
GCC_OPTZ := -Os
GCC_OPTZ := -O3

#  -Weffc++  -Wpedantic  -Wcast-qual
CWARN := -Wall -Wextra -W \
-Wdouble-promotion \
-Wfloat-equal \
-Wformat \
-Wformat-security \
-Wformat=2 \
-Wnarrowing \
-Wno-format-nonliteral \
-Wno-parentheses \
-Wpointer-arith \
-Wreturn-local-addr \
-Wshadow \
-Wundef \
-Wno-enum-compare \
-Wno-missing-field-initializers \
-Wno-sign-compare \
-Wno-unknown-pragmas \
-Wno-unused-function \
-Wno-unused-parameter \
-Wno-unused-value \
-Wno-unused-variable \


CXXWARN := -Woverloaded-virtual -Wold-style-cast -Wzero-as-null-pointer-constant

C_OPTS_COMMON  := $(GCC_OPTZ) $(CWARN) -funsigned-char $(TRAPV) $(CC_OUTPUT)
C_OPTS_LUA_REF := -I$(LUA_DIR)

#                 -fno-exceptions
USE_EXCEPTIONS := -fexceptions

LINK_OPTS_COMMON_ := $(USE_EXCEPTIONS) -fno-rtti $(STRIP) -Wl,-stats $(PLAT_LINK_OPTS)
ifdef DBG_BUILD
LINK_OPTS_COMMON := $(LINK_OPTS_COMMON_)
else

ifdef ComSpec
LINK_OPTS_COMMON := $(LINK_OPTS_COMMON_) -Wl,--dynamicbase
else
LINK_OPTS_COMMON := $(LINK_OPTS_COMMON_)
endif

endif

LINK_MAP = -Wl,--cref -Wl,-Map=$(subst .,_,$@).map
LINK_MAP :=-Wl,--cref -Wl,-Map=
LINK= gcc

#define LINK_MAP
#-Wl,-Map=$1
#endef

KEEPASM := -save-temps -fverbose-asm  # to get .S files
KEEPASM :=

ifdef ComSpec

PLAT_OBJS := \
 win32.o          \
 win32_api.o      \
 win32_clipbd.o   \
 win32_conin.o    \
 win32_conout.o   \
 win32_contit.o   \
 win32_filename.o \
 win32_process.o  \

else

PLAT_OBJS := \
 linux_api.o

endif

USE_PCRE := 1
export USE_PCRE
ifneq (0,$(USE_PCRE))
PCRE_OBJ := pcre_intf.o
PCRE_LIB := -lpcre
else
PCRE_OBJ :=
PCRE_LIB :=
endif

# 20140821 this flag needs to be used when _compiling_ .cpp files as well as when (w/gcc 4.9.1) using gcc to generate .d
CXX_D_FLAGS = -std=gnu++11 -DUSE_PCRE=$(USE_PCRE) $(APP_IN_DLL_CPP)

#####################################################################################################################
# Set variables used in GNU Make builtin rules (run `make -p > rules` in a dir w/o any makefile to see these rules).
# We use these default rules to compile .c and .cpp source files, plugging in our custom options via these variables:
# CPPFLAGS is used when compiling both .c and .cpp, CFLAGS only for .c, CXXFLAGS only for .cpp
CPPFLAGS = -DWINVER=0x0501
CFLAGS   = $(C_OPTS_COMMON) $(C_OPTS_DBG)
CXXFLAGS = $(C_OPTS_COMMON) $(CXXWARN) $(CXX_D_FLAGS) $(USE_EXCEPTIONS) -fno-rtti $(C_OPTS_LUA_REF) $(KEEPASM) $(C_OPTS_DBG)
#####################################################################################################################

LIBLUA := liblua.a
LIBS   := -static-libgcc -static-libstdc++ -lgcc -lboost_filesystem -lboost_system $(OS_LIBS) $(LUA_DIR)/$(LIBLUA) $(PCRE_LIB) #  -lmcheck (seems not to exist in MinGW)

CPP_OPTS:=

OBJS := \
 changers.o     \
 buildexecute.o \
 cmdidx.o       \
 cursor.o       \
 display.o      \
 fbuf.o         \
 fbuf_edit.o    \
 fbuf_undo.o    \
 filename.o     \
 fn_misc.o      \
 fn_text.o      \
 fname_gen.o    \
 getopt.o       \
 keys.o         \
 krbtree.o      \
 lua_edlib.o    \
 lua_intf.o     \
 macro.o        \
 main.o         \
 mark.o         \
 misc.o         \
 my_fio.o       \
 my_strutils.o  \
 os_services.o  \
 $(PCRE_OBJ)    \
 rm_util.o      \
 search.o       \
 stringlist.o   \
 switches.o     \
 sysbufs.o      \
 $(PLAT_OBJS)   \
 wnd.o

TGT:=k

ifdef APP_IN_DLL

ED_DLL :=$(TGT)x
EXE_TGTS = $(TGT)$(EXE_EXT) $(ED_DLL)$(DLL_EXT)
APP_IN_DLL_CPP := -DAPP_IN_DLL=1

else

ED_DLL :=
EXE_TGTS = $(TGT)$(EXE_EXT)
APP_IN_DLL_CPP :=

endif

CMDTBL_OUTPUTS := cmdtbl.h

# !!! in Lua-5.1/src/Makefile, PLAT=mingw, LUA_T=lua (not lua.exe)
LUA_T=.$(DIRSEP)lua$(EXE_EXT)

CLEAN_ARGS := $(OBJS) *.d *.s *.ii $(CMDTBL_OUTPUTS) _buildtime.o $(TGT)_res.o $(TGT).o *.map

ZAP_ARGS := $(EXE_TGTS) $(LUA_T)

all : tags

TAGS_CMDLN = ctags$(EXE_EXT) --totals=yes --excmd=number --c-types=cdefgmnstuv --fields=+K --file-tags=yes -R

tags : $(EXE_TGTS)
		$(TAGS_CMDLN)

$(CMDTBL_OUTPUTS): $(LUA_T) cmdtbl.dat
	$(LUA_T) cmdtbl.lua $(CMDTBL_ARG) < cmdtbl.dat

cleanliblua:
	cd $(LUA_DIR) && $(MAKE) clean

$(LUA_DIR)/$(LIBLUA):
	cd $(LUA_DIR) && $(MAKE) $(LIBLUA)

# !!! in Lua-5.1/src/Makefile, PLAT=mingw, `make lua` builds lua.exe aka $(LUA_T)
BLD_LUA_T = cd $(LUA_DIR)&&$(MAKE) clean&&$(MAKE) lua&&$(MV) $(LUA_T) ../..&&$(MAKE) clean
BLD_LUA_T = cd $(LUA_DIR)&&$(MAKE) lua&&$(MV) $(LUA_T) ../..

$(LUA_T):
	$(BLD_LUA_T)

clean:
	$(RM) $(CLEAN_ARGS)

zap: cleanliblua
	$(RM) $(ZAP_ARGS) $(CLEAN_ARGS)

ifdef ComSpec

WINDRES=$(TGT)_res.o

$(WINDRES): $(TGT).rc  # http://sourceware.org/binutils/docs/binutils/windres.html
	windres $< $@

else
WINDRES=
endif

ifdef APP_IN_DLL

$(TGT)$(EXE_EXT): $(TGT).o $(WINDRES)
	$(LINK) $^ $(LINK_MAP) -o $@ $(LINK_OPTS_COMMON) $(LINK_MAP)k.map
	@objdump -p $@ > $@.exp
	@$(LS_L) $@ $(LS_L_TAIL)

$(ED_DLL)$(DLL_EXT): $(OBJS) $(LUA_DIR)/$(LIBLUA)
	@$(LUA_T) datetime.lua > _buildtime.c&&$(COMPILE.c) _buildtime.c
	@echo linking $@&& g++ -shared -o $@ $(OBJS) _buildtime.o $(LIBS) $(LINK_OPTS_COMMON) $(LINK_MAP)kx.map
	@objdump -p $@ > $@.exp
	@$(LS_L) $@ $(LS_L_TAIL)

else

$(TGT)$(EXE_EXT): $(OBJS) $(WINDRES)
	@$(LUA_T) datetime.lua > _buildtime.c&&$(COMPILE.c) _buildtime.c
	@echo linking $@&& g++ $^ $(LINK_MAP) -o $@ _buildtime.o $(LIBS) $(LINK_OPTS_COMMON) $(LINK_MAP)k.map
	@objdump -p $@ > $@.exp
	@$(LS_L) $@ $(LS_L_TAIL)

endif

#######################################################################################
# patch GNU make 4.0 (or nuwen GCC 11.6 distro) bug by replacing dflt .c compile rule

CC = gcc
export CC

# end patch
#######################################################################################

BLD_CXX = $(COMPILE.cpp) $(OUTPUT_OPTION) $<
BLD_CXX = @echo COMPILE.cpp $<&&$(COMPILE.cpp) $(OUTPUT_OPTION) $<

%.o: %.cpp
	$(BLD_CXX)

BLD_C = @echo COMPILE.c $<&&$(COMPILE.c) $(OUTPUT_OPTION) $<
BLD_C = $(COMPILE.c) $(OUTPUT_OPTION) $<

%.o: %.c
	$(BLD_C)

# GCC _alone_ FTW!  sed, intermed files NOT needed!  https://news.ycombinator.com/item?id=7700812
# both of these work:
BLD_CPP_to_D = gcc $(CXX_D_FLAGS) -MM -MF $@ $(C_OPTS_LUA_REF) $< -MT $@ -MT $(basename $<).o
BLD_CPP_to_D = gcc $(CXX_D_FLAGS) -MM -MF $@ $(C_OPTS_LUA_REF) $< -MT $@ -MT $*.o

%.d: %.cpp $(CMDTBL_OUTPUTS)
	@echo generating $*.d&&$(BLD_CPP_to_D)

ifneq "$(MAKECMDGOALS)" "zap"
ifneq "$(MAKECMDGOALS)" "clean"

  -include $(OBJS:.o=.d)

endif
endif

ed_core.h: $(CMDTBL_OUTPUTS)

.PHONY: kb_190351
kb_190351:
	gcc KB_190351_child.c -o KB_190351_child$(EXE_EXT)
	gcc KB_190351.c -o KB_190351$(EXE_EXT)

PANDOC_OPTS := -f markdown_github-hard_line_breaks-raw_html+inline_notes+pipe_tables --section-divs

khelp.html: khelp.txt
	./pdconv.lua > log&&./pandoc.bat khelp.md_ $(PANDOC_OPTS) -t html -s -o khelp.html&&khelp.html
