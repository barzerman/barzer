UNAME := $(shell uname -a | cut -d' ' -f1)

FLAGS := $(FLAGS) 
ifeq ($(IS64),yes)
	BITMODE=-m64
endif 
ifeq ($(IS32),yes)
	BITMODE=-m32
endif 

ifeq ($(UNAME),Darwin)
    C11LIB_SHIT=-stdlib=libc++
endif
C11=-std=c++11
#CC=/opt/local/bin/gcc-mp-4.7
CFLAGS := $(OPT) $(C11) $(C11LIB) $(BITMODE) $(FLAGS) -Wall -Wno-unneeded-internal-declaration -Wno-unused-variable -g -I. -fpic -I../ -I/usr/local/include -I/opt/local/include -Wno-parentheses
LIBNAME=libay.a
SHARED_LIBNAME=libay.so

objects=ay_geo.o ay_translit_ru.o ay_keymaps.o ay_ngrams.o ay_xml_util.o ay_snowball.o ay_utf8.o ay_cmdproc.o ay_shell.o ay_util.o ay_string_pool.o ay_util_time.o ay_logger.o

all: $(objects)
	$(AR) -r  $(LIBNAME) $(objects)
shared: $(objects)
	$(CC) $(C11) $(C11LIB) -shared -Wl,-soname,$(SHARED_LIBNAME) -o $(SHARED_LIBNAME) $(objects)
test: $(LIBNAME)
	$(CC) -o testay testay.cpp $(CFLAGS) -lstdc++ -L. -lay $(C11LIB)
.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@
rebuild: clean all
clean : 
	rm -f $(objects) $(LIBNAME)

