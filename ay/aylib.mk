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
C0X=-std=c++0x
CFLAGS := $(OPT) $(C0X) $(C11LIB) $(BITMODE) $(FLAGS) -Wall -g -I. -fpic -I../ -I/usr/local/include -I/opt/local/include -Wno-parentheses
LIBNAME=libay.a
SHARED_LIBNAME=libay.so

objects=ay_translit_ru.o ay_keymaps.o ay_ngrams.o ay_xml_util.o ay_snowball.o ay_utf8.o ay_cmdproc.o ay_shell.o ay_util.o ay_string_pool.o ay_util_time.o ay_logger.o

all: $(objects)
	$(AR) -r  $(LIBNAME) $(objects)
shared: $(objects)
	$(CC) $(C0X) $(C11LIB) -shared -Wl,-soname,$(SHARED_LIBNAME) -o $(SHARED_LIBNAME) $(objects)
test: $(LIBNAME)
	$(CC) -o testay testay.cpp $(CFLAGS) -L. -lay $(C11LIB)
clean: 
	rm -f $(objects) $(LIBNAME)

.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@
rebuild: clean all
