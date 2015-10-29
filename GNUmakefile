#
# Copyright 2015 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
#
# This file is part of K.
#
# K is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# K is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along
# with K.  If not, see <http:#www.gnu.org/licenses/>.
#

#
# When building on Windows, uses MinGW tools from http://nuwen.net/mingw.html
#
# When building on (*ubuntu) Linux,
#   . install_build_tools.sh
# to install the needed tools & libs, and prep this file to use them
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
$(info make $(MAKE_VERSION))

.SUFFIXES:
.PHONY: zap clean cleanliblua

# APP_IN_DLL = 1

# 'ifdef ComSpec' -> 'if building on Windows'
ifdef ComSpec
PLAT = mingw
export PLAT

# By default GNU make searches $(PATH) for an executable named 'sh' (in the
# Mingw case, 'sh.exe'), and if found, uses it as the shell which executes all
# child build command lines.  Msysgit (for some install variants) puts a
# directory "C:\Program Files (x86)\Git\bin", which contains an instance of
# sh.exe, in PATH.  This causes make to use msysgit's sh.exe when executing
# build steps, which (unless I decided to formally adopt Msysgit as a
# prerequisite build-tool for K) causes the K build to fail (since this makefle
# is written for the default MS shell, 'CMD.exe').  In the past I've renamed
# this file to sh_.exe and lived to tell the tale, but my use case for git
# (perhaps uniquely) did not, until recently, include `git clone` which fails
# if it cannot find (its) sh.exe.  Also I've found it useful to have this dir
# in PATH since these versions of the "unix utils" tend to be quite up-to-date
# and "come for free" since I religiously update msysgit on each release,
# renaming sh.exe to sh_.exe is no longer an option.  'SHELL=cmd' forces use of
# CMD.exe even if an sh.exe is present in $(PATH)

SHELL=cmd
export SHELL # inherited by child (recursive) makes such as that which builds $(LUA_T)

# dflt $(RM) for MinGW is rm -f
RM= del /F /Q
RM= rm -f
export RM
MV = move
export MV
EXE_EXT := .exe
export EXE_EXT
DLL_EXT := .dll
OS_LIBS := -lpsapi
PLAT_LINK_OPTS=-Wl,--enable-auto-image-base -Wl,--nxcompat
# LS_L_TAIL is from http://ss64.com/nt/dir.html (MS BATch programming and cmdline utils suck SO BAD!)
LS_L := dir
LS_L_TAIL := | FIND "/"
# MinGW _mostly_ works OK using / as dirsep, HOWEVER when specifying a path prefix to argv[0], cmd _requires_ DIRSEP==\ (fails if DIRSEP==/)
DIRSEP := \\
CPPFLAGS += -DWINVER=0x0501

else
PLAT = linux
export PLAT

MV = mv
export MV
RM= rm -f
export RM
EXE_EXT :=
DLL_EXT := .so
CPPFLAGS += -pthread
# NB: once certain C++ _compiles_ see CPPFLAGS, should remove -lpthread
OS_LIBS := -lncurses -lpthread
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
GCC_OPTZ := -O0
else
STRIP      := -s
C_OPTS_DBG :=
GCC_OPTZ := -Os
GCC_OPTZ := -O3
endif

LUA_DIR=lua-5.1/src

CC_OUTPUT := # --save-temp
# -fstack-protector    gens link error
# -fsanitize=undefined gens link errors (is supposed to work on Linux x64 w/GCC 4.9.2 only)
# -fstrict-overflow -Wstrict-overflow=2
TRAPV := -ftrapv

#  -Weffc++  -Wpedantic  -Wcast-qual
CWARN := -Wall -Wextra -W \
-Wcast-align \
-Wdouble-promotion \
-Wfloat-equal \
-Wformat \
-Wformat-security \
-Wformat=2 \
-Winit-self \
-Wlogical-op \
-Wmissing-include-dirs \
-Wnarrowing \
-Wno-enum-compare \
-Wno-format-nonliteral \
-Wno-missing-field-initializers \
-Wno-parentheses \
-Wno-sign-compare \
-Wno-unknown-pragmas \
-Wno-unused-function \
-Wno-unused-parameter \
-Wno-unused-value \
-Wno-unused-variable \
-Wpointer-arith \
-Wredundant-decls \
-Wreturn-local-addr \
-Wshadow \
-Wundef \


CXXWARN := \
-Wctor-dtor-privacy \
-Woverloaded-virtual \
-Wold-style-cast \
-Wzero-as-null-pointer-constant \

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
LINK= gcc

KEEPASM := -save-temps -fverbose-asm  # to get .S files
KEEPASM :=

ifdef ComSpec

PLAT_OBJS := \
 conin_win32.o    \
 conout_win32.o   \
 win32.o          \
 win32_api.o      \
 win32_clipbd.o   \
 win32_contit.o   \
 win32_filename.o \
 win32_process.o  \

else

PLAT_OBJS := \
 conout_ncurses.o \
 conin_ncurses.o \
 linux_api.o \
 linux_process.o

# http://stackoverflow.com/questions/14605362/linking-statically-only-boost-library-g
#
#   The only time g++ will make a decision (and take into account the
#   -Bstatic and -Bdynamic) is if it finds both in the same directory.  It
#   searches the directories in the given order, and when it finds a
#   directory which has either the static or the dynamic version of the
#   library, it stops.  And if only one version is present, it uses that one,
#   regardless.
#
BOOST_LIBS := -Wl,-Bstatic -lboost_filesystem -lboost_system -Wl,-Bdynamic

endif

USE_PCRE := 1
export USE_PCRE
ifneq "0" "$(USE_PCRE)"
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
CFLAGS   = $(C_OPTS_COMMON) $(C_OPTS_DBG)
CXXFLAGS = $(C_OPTS_COMMON) $(CXXWARN) $(CXX_D_FLAGS) $(USE_EXCEPTIONS) -fno-rtti $(C_OPTS_LUA_REF) $(KEEPASM) $(C_OPTS_DBG)
#####################################################################################################################

LIBLUA := $(LUA_DIR)/liblua.a
LIBS   := -static-libgcc -static-libstdc++ -lgcc $(BOOST_LIBS) $(OS_LIBS) $(LIBLUA) $(PCRE_LIB) #  -lmcheck (seems not to exist in MinGW)

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

UNBUILT_RLS_FILES = .krsrc k.luaedit k.filesettings strict.lua user.lua README.md menu.lua show.lua tu.lua util.lua re.lua

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

THISDIR := .$(DIRSEP)

# !!! in Lua-5.1/src/Makefile, PLAT=mingw, LUA_T=lua (not lua.exe)
LUA_T=$(THISDIR)lua$(EXE_EXT)

CLEAN_ARGS = $(OBJS) *.d *.s *.ii $(CMDTBL_OUTPUTS) _buildtime.o $(TGT)_res.o $(TGT).o *.map $(RLS_PKG_FILES) *_unittest *_unittest.o

ZAP_ARGS := $(EXE_TGTS) $(LUA_T)

all : tags

print-%: ; @echo $* = '$($*)' from $(origin $*)

.PHONY: printvars
printvars:
	$(foreach V,$(sort $(.VARIABLES)), \
	  $(if \
	     $(filter-out environment% default automatic,$(origin $V)), \
	       $(info $V=$($V) ($(value $V))) \
	  ) \
	)


TAGS_CMDLN = ctags$(EXE_EXT) --totals=yes --excmd=number --c-types=cdefgmnstuv --fields=+K --file-tags=yes -R

tags : $(EXE_TGTS)
	$(TAGS_CMDLN)

$(CMDTBL_OUTPUTS): $(LUA_T) cmdtbl.dat bld_cmdtbl.lua
	$(LUA_T) bld_cmdtbl.lua $(PLAT) < cmdtbl.dat

cleanliblua:
	cd $(LUA_DIR) && $(MAKE) clean

$(LIBLUA):
	cd $(LUA_DIR) && $(MAKE) liblua.a

# !!! in Lua-5.1/src/Makefile, PLAT=mingw, `make lua` builds lua.exe aka $(LUA_T)
BLD_LUA_T = cd $(LUA_DIR)&&$(MAKE) clean&&$(MAKE) test&&$(MV) $(LUA_T) ../..&&$(MAKE) clean
BLD_LUA_T = cd $(LUA_DIR)&&$(MAKE) test&&$(MV) $(LUA_T) ../..

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

RLS_FILES = $(BUILT_RLS_FILES) $(UNBUILT_RLS_FILES)

RLS_PKG_FILES = $(TGT)_rls.7z $(TGT)_rls.exe

.PHONY: rls
rls: $(RLS_FILES)
ifdef ComSpec
	7z a      $(TGT)_rls.7z  $(RLS_FILES)
	7z a -sfx $(TGT)_rls.exe $(RLS_FILES)
else
	GZIP=-9 tar -zcvf $(TGT)_rls.tgz $(RLS_FILES)
endif

# phrases used in linking recipes
BLD_TIME_OBJ = @$(LUA_T) bld_datetime.lua > _buildtime.c&&$(COMPILE.c) _buildtime.c
LS_BINARY = $(LS_L) $@ $(LS_L_TAIL)
OBJDUMP_BINARY = echo objdumping $@&& objdump -p $@ > $@.exp
SHOW_BINARY = @$(OBJDUMP_BINARY) && $(LS_BINARY)

ifdef APP_IN_DLL

BUILT_RLS_FILES = $(TGT)$(EXE_EXT) $(ED_DLL)$(DLL_EXT)

$(TGT)$(EXE_EXT): $(TGT).o $(WINDRES)
	$(LINK.cpp) $^ -o $@ $(LINK_OPTS_COMMON) $(LINK_MAP)
	$(SHOW_BINARY)

$(ED_DLL)$(DLL_EXT): $(OBJS) $(LIBLUA)
	$(BLD_TIME_OBJ)
	@echo linking $@&& $(CXX) -shared -o $@ $(OBJS) _buildtime.o $(LIBS) $(LINK_OPTS_COMMON) $(LINK_MAP)
	$(SHOW_BINARY)

else

BUILT_RLS_FILES = $(TGT)$(EXE_EXT)

LINK_EXE_RAW = $(CXX) $^ -o $@ _buildtime.o $(LIBS) $(LINK_OPTS_COMMON) $(LINK_MAP) 2>link.errs || $(LUA_T) bld_link_unref.lua <link.errs
LINK_EXE = $(LINK_EXE_RAW)
LINK_EXE = @echo linking $@&& $(LINK_EXE_RAW)

$(TGT)$(EXE_EXT): $(OBJS) $(LIBLUA) $(WINDRES)
	$(BLD_TIME_OBJ)
	$(LINK_EXE)
	$(SHOW_BINARY)

endif

#######################################################################################
# patch GNU make 4.0 (or nuwen GCC 11.6 distro) bug by replacing dflt .c compile rule

ifeq "$(PLAT)" "mingw"

CC = gcc
export CC

endif

# end patch
#######################################################################################

ifeq "" ""
BLD_CXX = @echo COMPILE.cpp $<&&$(COMPILE.cpp) $(OUTPUT_OPTION) $<
BLD_C   = @echo COMPILE.c $<&&$(COMPILE.c) $(OUTPUT_OPTION) $<
else
BLD_CXX = $(COMPILE.cpp) $(OUTPUT_OPTION) $<
BLD_C   = $(COMPILE.c) $(OUTPUT_OPTION) $<
endif

%.o: %.cpp
	$(BLD_CXX)

%.o: %.c
	$(BLD_C)

# GCC _alone_ FTW!  sed, intermed files NOT needed!  https://news.ycombinator.com/item?id=7700812
# both of these work:
BLD_CPP_to_D = $(CC) $(CXX_D_FLAGS) $(CPPFLAGS) -MM -MF $@ $(C_OPTS_LUA_REF) $< -MT $@ -MT $(basename $<).o
BLD_CPP_to_D = $(CC) $(CXX_D_FLAGS) $(CPPFLAGS) -MM -MF $@ $(C_OPTS_LUA_REF) $< -MT $@ -MT $*.o

%.d: %.cpp $(CMDTBL_OUTPUTS)
	@echo generating $*.d&&$(BLD_CPP_to_D)

ifneq "$(MAKECMDGOALS)" "zap"
ifneq "$(MAKECMDGOALS)" "clean"

  -include $(patsubst %.o,%.d,$(OBJS))

endif
endif

ed_core.h: $(CMDTBL_OUTPUTS)

.PHONY: kb_190351
kb_190351:
	gcc KB_190351_child.c -o KB_190351_child$(EXE_EXT)
	gcc KB_190351.c -o KB_190351$(EXE_EXT)

PANDOC_OPTS := -f markdown_github-hard_line_breaks-raw_html+inline_notes+pipe_tables --section-divs

khelp.html: khelp.txt
	$(THISDIR)pdconv.lua > log
	$(THISDIR)pandoc.bat khelp.md_ $(PANDOC_OPTS) -t html -s -o khelp.html
	khelp.html


.PHONY: run_unittests run_krbtree_unittest run_dlink_unittest

run_unittests: run_krbtree_unittest run_dlink_unittest

run_dlink_unittest: dlink_unittest$(EXE_EXT)
	$(THISDIR)dlink_unittest$(EXE_EXT)

dlink_unittest$(EXE_EXT): CXXFLAGS += -Werror
dlink_unittest$(EXE_EXT): dlink_unittest.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

run_krbtree_unittest: krbtree_unittest$(EXE_EXT)
	$(THISDIR)krbtree_unittest$(EXE_EXT) < COPYING

krbtree_unittest$(EXE_EXT): krbtree_unittest.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

krbtree_unittest.o: CPPFLAGS += -DUNIT_TEST_KRBTREE
krbtree_unittest.o: CXXFLAGS += -Werror
krbtree_unittest.o: krbtree.cpp
	$(BLD_CXX)
