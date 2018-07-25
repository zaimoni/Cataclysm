# Makefile for OS directory
# assumes GNU make

.SUFFIXES:
.SUFFIXES: .cpp .a .o

include ../POSIX.inc
include ../POSIX.dep

CFLAGS := $(CFLAGS_CORE) $(CFLAGS_MICROOPT)
CXXFLAGS := $(CFLAGS)

LIBS := libz_log_adapter.a

# GNU targets we know about
all: $(LIBS)

clean:
	rm -f *.a *.o

host_install: $(addprefix ../../$(host_libdir)/, $(LIBS))

# rules
log_adapter_inform.o: log_adapter_inform.cpp ../Pure.C/format_util.c
log_adapter_inc_inform.o: log_adapter_inc_inform.cpp ../Pure.C/format_util.c
log_adapter_log.o: log_adapter_log.cpp ../Pure.C/format_util.c

.cpp.o:
	$(CXX) -c $< $(CXXFLAGS) $(ARCH_FLAGS) -o $*.o

libz_log_adapter.a: log_adapter_inc_inform.o log_adapter_inform.o log_adapter_log.o
	$(AR) $(ARFLAGS) libz_log_adapter.a log_adapter_inc_inform.o log_adapter_inform.o log_adapter_log.o

../../$(host_libdir)/libz_log_adapter.a: libz_log_adapter.a
	cp libz_log_adapter.a ../../$(host_libdir)/libz_log_adapter.a
