#
# Normally, Xcode builds go under a "build" subdirectory.  In particular, a
# component being built has its intermediate files go into
# "builds/COMPONENT/CONFIG/COMPONENT/...".  The final output goes into
# "build/CONFIG".
#
# This makefile fragments puts the output into "output" subdirectory and
# generates intermediate files in-place (for now).
#

# PROGRAM=<Name>
# TYPE={dsplug,app}
#
# INFO=<Path to Info.plist>
#
# SOURCES = <files from PBXResourcesBuildPhase section (w/subdirs as needed)>
# REZ_FILES = <files from PBXRezBuildPhase section>
# RESOURCES = <files from PBXResourcesBuildPhase section>
#
# FRAMEWORKS=<framework names>
#
# LOCAL_CFLAGS=<whatever CFLAGS you want>
# LOCAL_LDFLAGS=<whatever LDFLAGS you want>
#
# include Makefile.inc

TOOL_RESMERGER=/Developer/Tools/ResMerger

TYPE_CFLAGS:=
TYPE_LDFLAGS:=
TYPE_PKGINFO_FLAGS:=

ifeq ($(PROGRAM),)
$(error PROGRAM is not defined)
endif

# GNU make 3.81 added support for "else CONDITIONAL" syntax.  GNU make 3.80
# requires else nesting.

ifeq ($(TYPE),)
$(error TYPE is not defined)
else
ifeq ($(TYPE),app)
TYPE_CFLAGS:=-mdynamic-no-pic
TYPE_PKGINFO_FLAGS=--genpkginfo $(OUT_PKGINFO)
else
ifeq ($(TYPE),dsplug)
TYPE_LDFLAGS:=-bundle
else
$(error Invalid TYPE: $(TYPE))
endif
endif
endif

ifeq ($(INFO),)
$(error INFO is not defined)
endif

ifeq ($(SOURCES),)
$(error SOURCES is not defined)
endif

ifeq ($(LANGUAGE),)
LANGUAGE=English
endif

RESOURCES_NEUTRAL=$(filter %.icns,$(RESOURCES))
RESOURCES_LOCALIZE=$(filter %.strings %.nib,$(RESOURCES))
RESOURCES_UNKNOWN=$(filter-out %.icns %.strings %.nib,$(RESOURCES))

OUT_DIR=output

OUT_CONTENTS_DIR=$(OUT_DIR)/$(PROGRAM).$(TYPE)/Contents
OUT_PROGRAM_DIR=$(OUT_CONTENTS_DIR)/MacOS
OUT_INFO_DIR=$(OUT_CONTENTS_DIR)
OUT_RESOURCES_DIR=$(OUT_CONTENTS_DIR)/Resources
OUT_REZ_DIR=$(OUT_RESOURCES_DIR)

OBJS:=$(SOURCES)
OBJS:=$(OBJS:%.c=%.o)
OBJS:=$(OBJS:%.cp=%.o)
OBJS:=$(OBJS:%.cpp=%.o)

OUT_PROGRAM=$(OUT_PROGRAM_DIR)/$(PROGRAM)
OUT_INFO=$(OUT_INFO_DIR)/Info.plist
OUT_PKGINFO=$(OUT_INFO_DIR)/PkgInfo

IN_RESOURCES_NEUTRAL=$(RESOURCES_NEUTRAL)
OUT_RESOURCES_NEUTRAL_DIR=$(OUT_RESOURCES_DIR)
OUT_RESOURCES_NEUTRAL=$(addprefix $(OUT_RESOURCES_NEUTRAL_DIR)/,$(RESOURCES_NEUTRAL))

IN_RESOURCES_LOCALIZE=$(addprefix $(LANGUAGE).lproj/,$(RESOURCES_LOCALIZE))
OUT_RESOURCES_LOCALIZE_DIR=$(OUT_RESOURCES_DIR)/$(LANGUAGE).lproj

OUT_REZ=
ifneq ($(REZ_FILES),)
OUT_REZ=$(OUT_REZ_DIR)/$(PROGRAM).rsrc
endif

OUT_RESOURCES_LOCALIZE=$(addprefix $(OUT_RESOURCES_LOCALIZE_DIR)/,$(RESOURCES_LOCALIZE))

#-pipe -Wno-trigraphs -fno-common -fpascal-strings -Wmost -Wno-four-char-constants -Wno-unknown-pragmas
STD_CFLAGS=-pipe -fno-common -fpascal-strings -Wno-four-char-constants -Wall -Werror -mmacosx-version-min=10.4 -fvisibility-inlines-hidden
#STD_CFLAGS:=-arch i386 $(STD_CFLAGS)

#STD_LDFLAGS=-Wl,-Y,1455

COMPILE_FLAGS=-I. $(STD_CFLAGS) $(TYPE_CFLAGS) $(LOCAL_CFLAGS)
LINK_FLAGS=$(TYPE_LDFLAGS) $(addprefix -framework ,$(FRAMEWORKS)) $(LOCAL_LDFLAGS)

all: $(OUT_INFO) $(OUT_REZ) $(OUT_RESOURCES_NEUTRAL) $(OUT_RESOURCES_LOCALIZE) $(OUT_PROGRAM)

clean:
	rm -rf $(OUT_DIR)
	rm -f *.o

.PHONY: all clean
.SUFFIX: .c .cpp .o

$(OUT_INFO): $(INFO)
	mkdir -p $(OUT_INFO_DIR)
	./process-info-plist.pl $(TYPE_PKGINFO_FLAGS) -v $(MAIN_VERSION) -o "$@" $(INFO)

ifneq ($(OUT_REZ),)
$(OUT_REZ): $(REZ_FILES)
	mkdir -p $(OUT_REZ_DIR)
	$(TOOL_RESMERGER) -dstIs DF $(REZ_FILES) -o $(OUT_REZ)
endif

ifneq ($(OUT_RESOURCES_NEUTRAL),)
$(OUT_RESOURCES_NEUTRAL): $(IN_RESOURCES_NEUTRAL)
	mkdir -p $(OUT_RESOURCES_NEUTRAL_DIR)
	rsync -a $? $(OUT_RESOURCES_NEUTRAL_DIR)/.
endif

ifneq ($(OUT_RESOURCES_LOCALIZE),)
$(OUT_RESOURCES_LOCALIZE): $(IN_RESOURCES_LOCALIZE)
	mkdir -p $(OUT_RESOURCES_LOCALIZE_DIR)
	rsync -a $? $(OUT_RESOURCES_LOCALIZE_DIR)/.
endif

$(OUT_PROGRAM): $(OBJS)
	mkdir -p $(OUT_PROGRAM_DIR)
	g++-4.0 $(LINK_FLAGS) -o "$@" $(OBJS)

%.o : %.c
	gcc $(COMPILE_FLAGS) -c -o $@ $<

%.o : %.cp
	g++-4.0 -x c++ $(COMPILE_FLAGS) -c -o $@ $<

%.o : %.cpp
	g++-4.0 -x c++ $(COMPILE_FLAGS) -c -o $@ $<
