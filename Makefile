# comment these to toggle them as one sees fit.
# DEBUG is best turned on if you plan to debug in gdb -- please do!
# PROFILE is for use with gprof or a similar program -- don't bother generally
WARNINGS = -Wall -Wextra\
  -Wno-switch -Wno-sign-compare -Wno-unused-variable -Wno-maybe-uninitialized\
  -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-parameter
#DEBUG = -g
#PROFILE = -pg
OTHERS = -O3 -std=gnu++17 -MMD -MP

TARGET1 = cataclysm
TARGET2 = socrates-daimon

CXX = g++
CFLAGS = $(WARNINGS) $(DEBUG) $(PROFILE) $(OTHERS)

ifeq ($(shell uname -o), Msys)
LDFLAGS = -static -lgdi32
else
LDFLAGS = -lncurses
endif
LDFLAGS += -pthread -Llib/host -lz_format_util -lz_stdio_log -lz_stdio_c

ZAIMONI_HEADERS = Zaimoni.STL/Pure.C/comptest.h
ZAIMONI_LIBS = lib/host/libz_format_util.a lib/host/libz_stdio_c.a lib/host/libz_stdio_log.a

SOURCES1 = $(sort $(filter-out html.cpp socrates-daimon.cpp stdafx.cpp,$(wildcard *.cpp)))
ODIR1 = obj_cataclysm
OBJS1 = $(addprefix $(ODIR1)/,$(SOURCES1:.cpp=.o))

SOURCES2 = calendar.cpp catacurse.cpp color.cpp html.cpp item.cpp itypedef.cpp\
  json.cpp mapdata.cpp mtypedef.cpp options.cpp output.cpp pldata.cpp\
  recent_msg.cpp saveload.cpp skill.cpp socrates-daimon.cpp trapdef.cpp
ODIR2 = obj_socrates_daimon
OBJS2 = $(addprefix $(ODIR2)/,$(SOURCES2:.cpp=.o))


# Main Targets
.PHONY: all clean
all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1) $(ZAIMONI_LIBS)
	$(CXX) -o $@ $(CFLAGS) -DCATACLYSM $(OBJS1) $(LDFLAGS)

$(TARGET2): $(OBJS2) $(ZAIMONI_LIBS)
	$(CXX) -o $@ $(CFLAGS) -DSOCRATES_DAIMON $(OBJS2) $(LDFLAGS)

$(OBJS1): | $(ZAIMONI_HEADERS) $(ODIR1)
$(OBJS2): | $(ZAIMONI_HEADERS) $(ODIR2)

$(ODIR1) $(ODIR2):
	mkdir $@

$(ODIR1)/%.o: %.cpp
	$(CXX) $(CFLAGS) -DCATACLYSM -c $< -o $@

$(ODIR2)/%.o: %.cpp
	$(CXX) $(CFLAGS) -DSOCRATES_DAIMON -c $< -o $@

clean:
	rm -f $(TARGET1) $(TARGET2) $(ODIR1)/*.[od] $(ODIR2)/*.[od]


# Zaimoni.STL header & library builds
.PHONY: libs clean_libs
libs:
	$(MAKE) -C Zaimoni.STL/Pure.C/ host_install
	$(MAKE) -C Zaimoni.STL/Pure.C/stdio.log/ host_install

clean_libs:
	rm -f $(ZAIMONI_LIBS)
	$(MAKE) -C Zaimoni.STL/Pure.C/ clean
	$(MAKE) -C Zaimoni.STL/Pure.C/stdio.log/ clean

ifneq ($(filter grouped-target,$(.FEATURES)),)
# GNU make 4.3 feature
$(ZAIMONI_HEADERS) $(ZAIMONI_LIBS) &:
	$(MAKE) -C Zaimoni.STL/Pure.C/ host_install
	$(MAKE) -C Zaimoni.STL/Pure.C/stdio.log/ host_install
else
$(ZAIMONI_HEADERS) $(ZAIMONI_LIBS):
# don't try to build Zaimoni.STL targets by default because
# it won't work properly with parallel builds (-j option)
endif


-include $(OBJS1:.o=.d)
-include $(OBJS2:.o=.d)
