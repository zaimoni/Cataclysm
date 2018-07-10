# GNU compatibility core
# prefix = ...
# exec_prefix = $(prefix)
# bindir = $(exec_prefix)/bin
# includedir = $(exec_prefix)/include
# libdir = $(exec_prefix)/lib

# install host-built libraries here
host_libdir = lib

AR = ar
CC = gcc -std=gnu11
CXX = g++ -std=c++11

EXEEXT = exe
OBJEXT = o

ARCH_FLAGS = 

# compiler options
CFLAGS_CORE =  -Os -Wall -finline
CFLAGS_STRICT =  -fargument-alias --pedantic
CFLAGS_MICROOPT = -Wextra -Winline -Wdisabled-optimization --param max-inline-insns-single=500 -c -pipe -DNDEBUG

# archiver options
ARFLAGS = -sru
