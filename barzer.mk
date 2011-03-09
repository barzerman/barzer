CFLAGS := -I/opt/local/include -Wall -g -I. -I./ay -j2
LINKFLAGS := 
BINARY=barzer.exe
libs = ay/aylib.a /opt/local/lib/boost/libboost_system.a
ECHO = echo
objects = \
barzer.o \
barzer_shell.o\
barzer_parse_types.o \
barzer_lexer.o \
barzer_parse.o \
barzer_language.o \
barzer_dtaindex.o \
barzer_server.o \
barzer_token.o \
lg_en/barzer_en_lex.o \
lg_ru/barzer_ru_lex.o

all: $(objects)
	c++ $(LINKFLAGS) -o  $(BINARY) $(libs) $(objects)
clean: 
	rm -f $(objects) $(BINARY)
aylib: 
	cd ay; make -f aylib.mk rebuild
.cpp.o:
	c++  -c $(CFLAGS) $< -o $@
rebuild: clean aylib all
