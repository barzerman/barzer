CFLAGS := -m64 -g -I. -I./ay
LINKFLAGS := -m64
BINARY=barzer.exe
libs = ay/aylib.a
ECHO = echo
objects = barzer.o barzer_shell.o barzer_parse_types.o \
barzer_lexer.o barzer_parse.o 

all: $(objects)
	c++ $(LINKFLAGS) -o  $(BINARY) $(libs) $(objects)
clean: 
	rm -f $(objects) $(BINARY)
aylib: 
	cd ay; make -f aylib.mk rebuild
.cpp.o:
	c++  -c $(CFLAGS) $< -o $@
rebuild: clean aylib all
