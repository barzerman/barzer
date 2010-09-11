CFLAGS := -I.
BINARY=barzer.exe
libs = ay/aylib.a
objects = barzer.o

all: $(objects)
	c++ -o  $(BINARY) $(libs) $(objects)
clean: 
	rm -f $(objects) $(BINARY)

.cpp.o:
	c++  -c $(CFLAGS) $< -o $@
