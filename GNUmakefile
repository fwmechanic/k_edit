#!make
#
# Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
# When building on Windows, uses
#  * GCC from http://nuwen.net/mingw.html
#     * needs to be in PATH
#     * with equivalent of its SET_DISTRO_PATHS.BAT executed
#  * 'git for Windows' for MinGW environment (including bash)
#     #### make MUST be run from 'git for Windows' bash shell,
#     ####        ***NOT*** from CMD.exe (Windows "DOS shell")
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

ifeq '$(strip $(SHELL))' ''
  ifneq '$(strip $(COMSPEC))' ''
    $(error COMSPEC defined but SHELL undefined; Git For Windows not installed?)
  else
    $(error SHELL not defined, cannot continue)
  endif
endif

OS := $(shell uname -o)
$(info uname -o says: '$(OS)')
ifeq '$(OS)' 'Msys'
  K_WINDOWS := 1
else
  ifeq '$(OS)' 'GNU/Linux'
  else
    ifneq '$(COMSPEC)' ''
      $(error COMSPEC defined but uname -o output != 'Msys'; Git For Windows not installed?)
    else
      $(error unknown/unsupported uname -o output, cannot continue)
    endif
  endif
endif

TGT:=k

# wrap -llib refs to make them link statically
LINK_LIB_STATIC = -Wl,-Bstatic $1 -Wl,-Bdynamic

ifdef K_WINDOWS

# CMDTBL_PLAT is ONLY to control cmdtbl processing
CMDTBL_PLAT = mingw

# to disable, _comment out_ next line!
# DBG_BUILD := x

#######################################################################################
# patch GNU make 4.0 (or nuwen-11.6+ GCC distro) disconnect by replacing dflt .c compile rule
CC = gcc
export CC
# end patch
#######################################################################################

EXE_EXT := .exe
DLL_EXT := .dll
OBJDUMP_BINARY = echo objdumping $@&& objdump -p $@ > $@.exp && grep "DLL Name:" $@.exp | grep -Fivf std.dynlib.mingw
OS_LIBS := -lpsapi -lbcrypt
PLAT_LINK_OPTS=-Wl,--enable-auto-image-base -Wl,--nxcompat
CPPFLAGS += -DWINVER=0x0501

else

# https://stackoverflow.com/a/43835225
NCURSES_WIDE := w

NCURSES_CONFIG := ncurses$(NCURSES_WIDE)5-config
NCURSES_CFLAGS := $(shell $(NCURSES_CONFIG) --cflags)
$(info NCURSES_CFLAGS: '$(NCURSES_CFLAGS)')
NCURSES_LIBS := $(shell $(NCURSES_CONFIG) --libs)
$(info NCURSES_LIBS: '$(NCURSES_LIBS)')

# CMDTBL_PLAT is ONLY to control cmdtbl processing
CMDTBL_PLAT = linux

# to disable, _comment out_ next line!
DBG_BUILD := x

EXE_EXT :=
DLL_EXT := .so
OBJDUMP_BINARY = echo "objdumping $@" && objdump -p $@ > $@.exp && readelf -d $@ | grep NEEDED | grep -Fvf std.dynlib.linux
# CPPFLAGS += -pthread $(NCURSES_CFLAGS)
CPPFLAGS += -pthread
# NB: once certain C++ _compiles_ see CPPFLAGS, should remove -lpthread
OS_LIBS := $(NCURSES_LIBS) -lpthread -ldl
PLAT_LINK_OPTS=

endif

# Generally I don't use a debugger, but when crashes occur, obtaining a stack
# trace is VERY helpful.  On Windows I use DrMinGW
# https://github.com/jrfonseca/drmingw/releases
# note that the version (32- vs. 64-bit) is _app-being-debugged_ specific!  And you can
# simultaneously install BOTH 32- and 64-bit versions of DrMinGW (and MUST if you want
# to be able to debug BOTH 32- and 64-bit versions of your app.
#
# You MUST rebuild K to get useful info from DrMinGW:
# 1. define DBG_BUILD (uncomment PLAT-specific line 'DBG_BUILD := 1' above)
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

ifdef DBG_BUILD
STRIP      :=
C_OPTS_DBG := -g -fno-omit-frame-pointer
GCC_OPTZ := -O0
else
STRIP      := -s
C_OPTS_DBG :=
GCC_OPTZ_ARCH ?= -march=native
GCC_OPTZ := -Os
GCC_OPTZ := -flto -O3 $(GCC_OPTZ_ARCH)
endif

LUA_DIR=lua-5.1/src

CC_OUTPUT := # --save-temp
# -fstack-protector    gens link error
# -fsanitize=undefined MinGW GCC 10.3 gens link errors for missing syms '__ubsan_handle_*' Linux might be better?
# -fstrict-overflow -Wstrict-overflow=2
TRAPV := -ftrapv

#  -Weffc++  -Wpedantic  -Wcast-qual
CWARN := -Wall -Werror -Wextra -W \
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
-Wno-logical-op \
-Wno-missing-field-initializers \
-Wno-parentheses \
-Wno-sign-compare \
-Wno-unknown-pragmas \
-Wno-unused-function \
-Wno-unused-parameter \
-Wno-unused-value \
-Wno-unused-variable \
-Wno-format-y2k \
-Wpointer-arith \
-Wredundant-decls \
-Wreturn-local-addr \
-Wshadow \
-Wundef \
-Wvla \


CXXWARN := \
-Wctor-dtor-privacy \
-Wno-noexcept-type \
-Woverloaded-virtual \
-Wold-style-cast \
-Wzero-as-null-pointer-constant \

C_OPTS_COMMON  := $(GCC_OPTZ) $(CWARN) -funsigned-char $(TRAPV) $(CC_OUTPUT)
CPPFLAGS += -I$(LUA_DIR)

#                 -fno-exceptions
USE_EXCEPTIONS := -fexceptions

LINK_OPTS_COMMON_ := $(USE_EXCEPTIONS) -fno-rtti $(STRIP) -Wl,-stats $(PLAT_LINK_OPTS)
ifdef DBG_BUILD
LINK_OPTS_COMMON := $(LINK_OPTS_COMMON_)
else

  ifdef K_WINDOWS
LINK_OPTS_COMMON := $(LINK_OPTS_COMMON_) -Wl,--dynamicbase
  else
LINK_OPTS_COMMON := $(LINK_OPTS_COMMON_)
  endif

endif

LINK_MAP = -Wl,--cref -Wl,-Map=$(subst .,_,$@).map
LINK= gcc

KEEPASM := -save-temps -fverbose-asm  # to get .S files
KEEPASM :=

ifdef K_WINDOWS
WINDRES_O=$(TGT)_res.o

PLAT_OBJS := \
 win32.o          \
 win32_api.o      \
 win32_clipbd.o   \
 win32_conin.o    \
 win32_conout.o   \
 win32_contit.o   \
 win32_filename.o \
 win32_process.o  \
 $(WINDRES_O)     \

else
WINDRES_O=

PLAT_OBJS := \
 linux_api.o      \
 linux_process.o  \
 ncurses_conin.o  \
 ncurses_conout.o \
 $(WINDRES_O)     \

ncurses_conin.o:  CPPFLAGS += $(NCURSES_CFLAGS)
ncurses_conout.o: CPPFLAGS += $(NCURSES_CFLAGS)

endif

# http://stackoverflow.com/questions/14605362/linking-statically-only-boost-library-g
#
#   The only time g++ will make a decision (and take into account the
#   -Bstatic and -Bdynamic) is if it finds both in the same directory.  It
#   searches the directories in the given order, and when it finds a
#   directory which has either the static or the dynamic version of the
#   library, it stops.  And if only one version is present, it uses that one,
#   regardless.
#
BOOST_LIBS := $(call LINK_LIB_STATIC,-lboost_filesystem -lboost_system)

export USE_PCRE := 1
ifneq "0" "$(USE_PCRE)"
export PCRE2_CODE_UNIT_WIDTH := 8
PCRE_OBJ := pcre_intf.o
PCRE_LIB := $(call LINK_LIB_STATIC,-lpcre2-$(PCRE2_CODE_UNIT_WIDTH))
CPPFLAGS += -DPCRE2_STATIC -DPCRE2_CODE_UNIT_WIDTH=$(PCRE2_CODE_UNIT_WIDTH)
else
PCRE_OBJ :=
PCRE_LIB :=
endif

# 20140821 this flag needs to be used when _compiling_ .cpp files as well as when (w/gcc 4.9.1) using gcc to generate .makedeps
CXX_D_FLAGS = -std=c++23 -DUSE_PCRE=$(USE_PCRE) $(APP_IN_DLL_CPP)

#####################################################################################################################
# Set variables used in GNU Make builtin rules (run `make -p > rules` in a dir w/o any makefile to see these rules).
# We use these default rules to compile .c and .cpp source files, plugging in our custom options via these variables:
# CPPFLAGS is used when compiling both .c and .cpp, CFLAGS only for .c, CXXFLAGS only for .cpp
CFLAGS   = $(C_OPTS_COMMON) $(C_OPTS_DBG)
CXXFLAGS = $(C_OPTS_COMMON) $(CXXWARN) $(CXX_D_FLAGS) $(USE_EXCEPTIONS) -fno-rtti $(KEEPASM) $(C_OPTS_DBG) -MMD -MF $*.makedeps
#####################################################################################################################

LUA_A := $(LUA_DIR)/liblua.a
LIBS   := -static-libgcc -static-libstdc++ -lgcc $(LUA_A) $(BOOST_LIBS) $(PCRE_LIB) $(OS_LIBS) #  -lmcheck (seems not to exist in MinGW)

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
 my_fio.o       \
 my_strutils.o  \
 os_services.o  \
 $(PCRE_OBJ)    \
 rm_util.o      \
 rsrc.o         \
 search.o       \
 stringlist.o   \
 switches.o     \
 switch_impl.o  \
 sysbufs.o      \
 tagfind.o      \
 $(PLAT_OBJS)   \
 wnd.o

UNBUILT_RLS_FILES = .krsrc k.luaedit k.filesettings strict.lua user.lua README.adoc menu.lua show.lua tu.lua util.lua re.lua

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

THISDIR := ./

LUA_T=$(LUA_DIR)/lua$(EXE_EXT)

CLEAN_ARGS = $(OBJS) *.makedeps *.s *.ii $(CMDTBL_OUTPUTS) _buildtime.o $(TGT)_res.o $(TGT).o *.map $(RLSPKG_FNMS) *_unittest *_unittest.o

ZAP_ARGS := $(EXE_TGTS) $(LUA_T) $(LUA_A)

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

# +p +x seem to do nothing
# +Z+s adds 'scope:' prefix to 'class:' and 'struct:' Extension fields (coalescing them into a single 'scope:' field)

TAGS_FIELDS=+K+z+S+l+n
#            K           Kind of tag as full name
#              z         Include the "kind:" key in kind field (use k or K) in tags output
#                S       Signature of routine (e.g. prototype or parameter list)
#                  l     Language of input file containing tag
#                    n   Line number of tag definition
TAGS_EXTRAS=+f
#            f           Include [a tag record] for the base file name of every input file

TAGS_CMDLN = ctags --totals=yes --tag-relative=yes --excmd=combine --fields=$(TAGS_FIELDS) --extras=$(TAGS_EXTRAS) --recurse
TAGS_CMDLN = ctags --totals=yes --tag-relative=yes --excmd=pattern --fields=$(TAGS_FIELDS) --extras=$(TAGS_EXTRAS) --recurse
# !!! SEE ALSO ctags.d/*.ctags

TAGS_FNM_BASE := .k_edit_tags

.PHONY: tags
tags : $(EXE_TGTS)
	@$(TAGS_CMDLN) --list-fields         > $(TAGS_FNM_BASE)_fields
	@$(TAGS_CMDLN) --list-extras         > $(TAGS_FNM_BASE)_extras
	@$(TAGS_CMDLN) --list-kinds-full=C++ > $(TAGS_FNM_BASE)_kinds
	$(TAGS_CMDLN) -o $(TAGS_FNM_BASE)

$(CMDTBL_OUTPUTS): $(LUA_T) cmdtbl.dat bld_cmdtbl.lua
	$(LUA_T) bld_cmdtbl.lua $(CMDTBL_PLAT) < cmdtbl.dat

cleanliblua:
	cd $(LUA_DIR) && $(MAKE) clean

echo:
	@echo "LUA_T = $(LUA_T)"

# !!! in Lua-5.1/src/Makefile, PLAT=mingw, `make lua` builds lua.exe aka $(LUA_T)
BLD_LUA_T = cd $(LUA_DIR)&&$(MAKE) clean&&$(MAKE) test&&mv $(LUA_T) ../..&&$(MAKE) clean
BLD_LUA_T = cd $(LUA_DIR)&&$(MAKE) test&&mv $(LUA_T) ../..
BLD_LUA_T = cd $(LUA_DIR)&&$(MAKE) clean&&$(MAKE) test&&mv $(LUA_T) ../..
BLD_LUA_T = $(MAKE) -C $(LUA_DIR) clean && $(MAKE) -C $(LUA_DIR) test && mv $(LUA_DIR)/$(LUA_T) .
BLD_LUA_T = $(MAKE) -C $(LUA_DIR) clean && $(MAKE) -C $(LUA_DIR) test

$(LUA_T):
	$(MAKE) -C $(LUA_DIR)

.PHONY: lua
lua:
	$(BLD_LUA_T)
	@ls -l $(LUA_T)

# $(LUA_A):  NOT needed since target $(LUA_T) builds $(LUA_A)
# 	$(MAKE) -C $(LUA_DIR) liblua.a

clean:
	$(RM) $(CLEAN_ARGS) $(LUA_T)
	$(MAKE) -C $(LUA_DIR) clean

zap: cleanliblua
	$(RM) $(ZAP_ARGS) $(CLEAN_ARGS)

ifdef K_WINDOWS
$(WINDRES_O): $(TGT).rc  # http://sourceware.org/binutils/docs/binutils/windres.html
	windres $< $@
endif

ifdef K_WINDOWS
RLSPKG_ARCHIVE = $(TGT)_rls.7z
RLSPKG_SFX = $(TGT)_rls.exe
else
RLSPKG_ARCHIVE = $(TGT)_rls.tgz
RLSPKG_SFX =
endif

RLSPKG_FNMS = $(RLSPKG_ARCHIVE) $(RLSPKG_SFX)

.PHONY: rls
rls: $(RLSPKG_FNMS)

# common phrases used in linking recipes
BLD_TIME_OBJ = @$(LUA_T) bld_datetime.lua > _buildtime.c&&$(COMPILE.c) _buildtime.c
LS_BINARY = ls -l $@
SHOW_BINARY = $(OBJDUMP_BINARY) ; $(LS_BINARY)

ifdef APP_IN_DLL

BUILT_RLS_FILES = $(TGT)$(EXE_EXT) $(ED_DLL)$(DLL_EXT)

$(TGT)$(EXE_EXT): $(TGT).o $(WINDRES_O)
	$(LINK.cpp) $^ -o $@ $(LINK_OPTS_COMMON) $(LINK_MAP)
	@$(SHOW_BINARY)

$(ED_DLL)$(DLL_EXT): $(OBJS)
	$(BLD_TIME_OBJ)
	@echo linking $@&& $(CXX) -shared -o $@ $(OBJS) _buildtime.o $(LIBS) $(LINK_OPTS_COMMON) $(LINK_MAP)
	@$(SHOW_BINARY)

else

BUILT_RLS_FILES = $(TGT)$(EXE_EXT)

LINK_EXE_RAW = $(CXX) $^ -o $@ _buildtime.o $(LIBS) $(LINK_OPTS_COMMON) $(LINK_MAP) 2>link.errs || ( $(LUA_T) bld_link_unref.lua <link.errs && cat link.errs && false )
LINK_EXE = $(LINK_EXE_RAW)
LINK_EXE = @echo linking $@&& $(LINK_EXE_RAW)

$(TGT)$(EXE_EXT): $(OBJS) $(WINDRES_O)
	$(BLD_TIME_OBJ)
	$(LINK_EXE)
	@$(SHOW_BINARY)

endif

RLS_FILES = $(BUILT_RLS_FILES) $(UNBUILT_RLS_FILES)

ifdef K_WINDOWS
$(RLSPKG_ARCHIVE): $(RLS_FILES) ; 7z a              $@ $^
$(RLSPKG_SFX)    : $(RLS_FILES) ; 7z a -sfx         $@ $^
else
$(RLSPKG_ARCHIVE): $(RLS_FILES) ; GZIP=-9 tar -zcvf $@ $^
endif


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

ifneq "$(MAKECMDGOALS)" "zap"
ifneq "$(MAKECMDGOALS)" "clean"

  include $(wildcard *.makedeps)  # if no .makedeps files exist: no harm, no foul

endif
endif

$(OBJS) : $(CMDTBL_OUTPUTS)  # force build of $(CMDTBL_OUTPUTS) when *.makedeps files absent (i.e. zap/clean + build)

.PHONY: kb_190351
kb_190351:
	gcc KB_190351_child.c -o KB_190351_child$(EXE_EXT)
	gcc KB_190351.c -o KB_190351$(EXE_EXT)

PANDOC_OPTS := -f markdown_github-hard_line_breaks-raw_html+inline_notes+pipe_tables --section-divs

khelp.html: khelp.txt
	./pdconv.lua > log
	./pandoc.bat khelp.md_ $(PANDOC_OPTS) -t html -s -o khelp.html
	khelp.html


UNITTEST_RUNTGTS = run_krbtree_unittest run_dlink_unittest run_unittest_tagfind
.PHONY: run_unittests $(UNITTEST_RUNTGTS)

run_unittests: $(UNITTEST_RUNTGTS)

run_dlink_unittest: dlink_unittest$(EXE_EXT)
	./dlink_unittest$(EXE_EXT)

dlink_unittest$(EXE_EXT): CXXFLAGS += -Werror
dlink_unittest$(EXE_EXT): dlink_unittest.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

run_krbtree_unittest: krbtree_unittest$(EXE_EXT)
	./krbtree_unittest$(EXE_EXT) < COPYING

krbtree_unittest$(EXE_EXT): krbtree_unittest.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

krbtree_unittest.o: CPPFLAGS += -DUNIT_TEST_KRBTREE
krbtree_unittest.o: CXXFLAGS += -Werror
krbtree_unittest.o: krbtree.cpp
	$(BLD_CXX)

run_unittest_tagfind: unittest_tagfind$(EXE_EXT)
	./test_tagfind $<

unittest_tagfind$(EXE_EXT): unittest_tagfind.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

unittest_tagfind.o: CPPFLAGS += -DUNITTEST
unittest_tagfind.o: tagfind.cpp
	$(BLD_CXX)
