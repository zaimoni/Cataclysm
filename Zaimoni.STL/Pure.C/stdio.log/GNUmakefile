# Makefile for pure.C directory
# assumes GNU make

.SUFFIXES:
.SUFFIXES: .c .a .o

include ../../POSIX.inc

CFLAGS := $(CFLAGS_CORE) $(CFLAGS_MICROOPT)

LIBS := libz_stdio_log.a

# GNU targets we know about
all: $(LIBS)

clean:
	rm -f *.a *.o

install: $(addprefix $(libdir)/, $(LIBS))
host_install: $(addprefix ../../../$(host_libdir)/, $(LIBS))

uninstall:
	rm -f $(addprefix $(libdir)/, $(LIBS))

# rules
.c.o:
	$(CC) -c $< $(CFLAGS) $(ARCH_FLAGS) -o $*.o

libz_stdio_log.a: fatal.o fatal_code.o inform.o inc_inform.o log.o
	$(AR) $(ARFLAGS) libz_stdio_log.a fatal.o fatal_code.o inform.o inc_inform.o log.o

../../../$(libdir)/libz_stdio_log.a: libz_stdio_log.a
	cp libz_stdio_log.a ../../../$(libdir)/libz_stdio_log.a

../../../$(host_libdir)/libz_stdio_log.a: libz_stdio_log.a
	cp libz_stdio_log.a ../../../$(host_libdir)/libz_stdio_log.a
	
