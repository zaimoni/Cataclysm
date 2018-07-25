# Makefile for pure.C directory
# assumes GNU make

.SUFFIXES:
.SUFFIXES: .c .a .o

include ../POSIX.inc

CFLAGS := -Os -Wall -finline -Wextra -Winline -Wdisabled-optimization --param max-inline-insns-single=500 -pipe -DNDEBUG

LIBS := libz_format_util.a libz_stdio_c.a

# GNU targets we know about
all: auto_int.h comptest.h $(LIBS)

clean:
	rm -f *.a *.o auto_int.h comptest.h

host_install: auto_int.h comptest.h $(addprefix ../../$(host_libdir)/, $(LIBS))

# rules
format_util.o: format_util.c format_util.h
stdio_c.o: stdio_c.c stdio_c.h comptest.h ../Logging.h ../Compiler.h

auto_int.h : int_probe.c
	$(CC) -oint_probe.exe $(CFLAGS) $(ARCH_FLAGS) int_probe.c
	./int_probe.exe > auto_int.h

comptest.h : comptest.in compile.test/exists_MS_io_h.c compile.test/realloc_0.c compile.test/exists_timespec.c
	cp comptest.in comptest.h
	-$(CC) -oexists_MS_io_h.exe $(CFLAGS) $(ARCH_FLAGS) compile.test/exists_MS_io_h.c
	-./exists_MS_io_h.exe >> comptest.h
	-rm exists_MS_io_h.exe
	-$(CC) -oexists_timespec.exe $(CFLAGS) $(ARCH_FLAGS) compile.test/exists_timespec.c
	-./exists_timespec.exe >> comptest.h
	-rm exists_timespec.exe
	-$(CC) -oexists_nanosleep.exe $(CFLAGS) -Werror=implicit-function-declaration $(ARCH_FLAGS) compile.test/exists_nanosleep.c
	-./exists_nanosleep.exe >> comptest.h
	-rm exists_nanosleep.exe
	-$(CC) -orealloc_0.exe $(CFLAGS) $(ARCH_FLAGS) compile.test/realloc_0.c
	-./realloc_0.exe >> comptest.h
	-rm realloc_0.exe

.c.o:
	$(CC) -c $< $(CFLAGS) $(ARCH_FLAGS) -o $*.o

libz_format_util.a: format_util.o
	$(AR) $(ARFLAGS) libz_format_util.a format_util.o

libz_stdio_c.a: stdio_c.o
	$(AR) $(ARFLAGS) libz_stdio_c.a stdio_c.o

../../$(host_libdir)/libz_format_util.a: libz_format_util.a
	cp libz_format_util.a ../../$(host_libdir)/libz_format_util.a
	
../../$(host_libdir)/libz_stdio_c.a: libz_stdio_c.a
	cp libz_stdio_c.a ../../$(host_libdir)/libz_stdio_c.a

