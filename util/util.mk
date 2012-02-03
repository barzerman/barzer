PYINCLUDE := $(shell python-config --includes)
PYLIBS := $(shell python-config --libs)

FLAGS := $(FLAGS) ${PYLIBS} -L/opt/local/lib -lboost_python -lstdc++ 
CFLAGS := $(CFLAGS) $(OPT) $(BITMODE) $(FLAGS) $(PYINCLUDE) -Wall -Wno-array-bounds -O3 -s -I. -fpic -I/opt/local/include 
LIBNAME=libbarzerutil.a
SHARED_LIBNAME=libbarzerutil.so
PYTHON_LIBNAME=python_util.so

objects = pybarzer.o umlaut.o python_util.o

all: deumlaut $(PYTHON_LIBNAME)
	echo "done"
deumlaut: $(objects)
	$(CC) -o deumlaut $(objects) $(FLAGS)
$(PYTHON_LIBNAME): $(objects)
	$(CC) -shared -o $(PYTHON_LIBNAME) $(objects) $(FLAGS) 
staticlib: $(objects)
	$(AR) -r  $(LIBNAME) $(objects)
sharedlib: $(objects)
	$(CC) -shared -Wl,-soname,$(SHARED_LIBNAME) -o $(SHARED_LIBNAME) $(objects)
clean: 
	rm -f $(objects) $(LIBNAME) $(SHARED_LIBNAME) $(PYTHON_LIBNAME)
.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@
rebuild: clean all
