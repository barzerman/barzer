FLAGS := $(FLAGS) 
ifeq ($(IS64),yes)
	BITMODE=-m64
endif 
ifeq ($(IS32),yes)
	BITMODE=-m32
endif 
CFLAGS := $(OPT) -std=c++0x $(BITMODE) $(FLAGS) -Wall -g -I. -fpic -I../ -I/usr/local/include -I/opt/local/include -Wno-parentheses
LIBNAME=libay.a
SHARED_LIBNAME=libay.so

objects=ay_translit_ru.o ay_keymaps.o ay_ngrams.o ay_xml_util.o ay_snowball.o ay_utf8.o ay_cmdproc.o ay_shell.o ay_util.o ay_string_pool.o ay_util_time.o ay_logger.o

all: $(objects)
	$(AR) -r  $(LIBNAME) $(objects)
shared: $(objects)
	$(CC) -shared -Wl,-soname,$(SHARED_LIBNAME) -o $(SHARED_LIBNAME) $(objects)
test: $(LIBNAME)
	$(CC) -o testay testay.cpp $(CFLAGS) -L. -lay -lstdc++
clean: 
	rm -f $(objects) $(LIBNAME)

.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@
rebuild: clean all
