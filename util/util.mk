FLAGS := $(FLAGS) -lstdc++
CFLAGS := $(CFLAGS) $(OPT) $(BITMODE) $(FLAGS) -Wall -g -I. -fpic
LIBNAME=libbarzerutil.a
SHARED_LIBNAME=libbarzerutil.so

objects = umlaut.o

all: umlaut
	echo "done"
umlaut: $(objects)
	$(CC) -o deumlaut $(objects) $(FLAGS)
staticlib: $(objects)
	$(AR) -r  $(LIBNAME) $(objects)
sharedlib: $(objects)
	$(CC) -shared -Wl,-soname,$(SHARED_LIBNAME) -o $(SHARED_LIBNAME) $(objects)
clean: 
	rm -f $(objects) $(LIBNAME) $(SHARED_LIBNAME)
.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@
rebuild: clean all
